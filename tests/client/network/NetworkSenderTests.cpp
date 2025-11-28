#include "input/InputBuffer.hpp"
#include "input/InputMapper.hpp"
#include "network/InputPacket.hpp"
#include "network/NetworkSender.hpp"
#include "network/UdpSocket.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace
{
    IpEndpoint ensureLoopback(IpEndpoint ep)
    {
        if (ep.addr == std::array<std::uint8_t, 4>{0, 0, 0, 0}) {
            ep.addr = {127, 0, 0, 1};
        }
        return ep;
    }
} // namespace

TEST(NetworkSender, SendsInputPacketToRemote)
{
    UdpSocket listener;
    ASSERT_TRUE(listener.open(IpEndpoint::v4(0, 0, 0, 0, 0)));
    listener.setNonBlocking(true);
    auto listenEp = ensureLoopback(listener.localEndpoint());

    InputBuffer buffer;
    NetworkSender sender(buffer, listenEp, 99, std::chrono::milliseconds(5));
    ASSERT_TRUE(sender.start());

    InputCommand cmd{};
    cmd.flags      = InputMapper::UpFlag | InputMapper::FireFlag;
    cmd.sequenceId = 7;
    cmd.posX       = 12.5F;
    cmd.posY       = -3.25F;
    cmd.angle      = 90.0F;
    buffer.push(cmd);

    bool received = false;
    auto start    = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(500)) {
        IpEndpoint src{};
        std::array<std::uint8_t, 128> buf{};
        auto res = listener.recvFrom(buf.data(), buf.size(), src);
        if (res.ok()) {
            auto pkt = InputPacket::decode(buf.data(), res.size);
            if (pkt && pkt->playerId == 99 && pkt->flags == cmd.flags) {
                EXPECT_FLOAT_EQ(pkt->x, cmd.posX);
                EXPECT_FLOAT_EQ(pkt->y, cmd.posY);
                EXPECT_FLOAT_EQ(pkt->angle, cmd.angle);
                received = true;
                break;
            }
        } else if (res.error == UdpError::WouldBlock || res.error == UdpError::Interrupted) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } else {
            break;
        }
    }

    sender.stop();
    EXPECT_TRUE(received);
}

TEST(NetworkSender, DoesNotSendWithoutCommands)
{
    UdpSocket listener;
    ASSERT_TRUE(listener.open(IpEndpoint::v4(0, 0, 0, 0, 0)));
    listener.setNonBlocking(true);
    auto listenEp = ensureLoopback(listener.localEndpoint());

    InputBuffer buffer;
    NetworkSender sender(buffer, listenEp, 1, std::chrono::milliseconds(5));
    ASSERT_TRUE(sender.start());

    bool received = false;
    auto start    = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(120)) {
        IpEndpoint src{};
        std::array<std::uint8_t, 128> buf{};
        auto res = listener.recvFrom(buf.data(), buf.size(), src);
        if (res.ok()) {
            received = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    sender.stop();
    EXPECT_FALSE(received);
}

TEST(NetworkSender, ReportsErrorOnSendFailure)
{
    InputBuffer buffer;
    std::atomic<bool> gotError = false;
    NetworkSender sender(buffer, IpEndpoint::v4(0, 0, 0, 0, 0), 1, std::chrono::milliseconds(5),
                         IpEndpoint::v4(0, 0, 0, 0, 0), [&](const IError& err) {
                             (void) err;
                             gotError = true;
                         });

    ASSERT_TRUE(sender.start());
    InputCommand cmd{};
    cmd.flags = InputMapper::FireFlag;
    buffer.push(cmd);

    auto start = std::chrono::steady_clock::now();
    while (!gotError && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(200)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    sender.stop();
    EXPECT_TRUE(gotError);
}

TEST(NetworkSender, SendsMultipleCommandsInOrder)
{
    UdpSocket listener;
    ASSERT_TRUE(listener.open(IpEndpoint::v4(0, 0, 0, 0, 0)));
    listener.setNonBlocking(true);
    auto listenEp = ensureLoopback(listener.localEndpoint());

    InputBuffer buffer;
    NetworkSender sender(buffer, listenEp, 5, std::chrono::milliseconds(2));
    ASSERT_TRUE(sender.start());

    InputCommand first{};
    first.flags      = InputMapper::LeftFlag;
    first.sequenceId = 1;
    InputCommand second{};
    second.flags      = InputMapper::RightFlag;
    second.sequenceId = 2;
    buffer.push(first);
    buffer.push(second);

    std::vector<std::uint16_t> receivedFlags;
    auto start = std::chrono::steady_clock::now();
    while (receivedFlags.size() < 2 && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(500)) {
        IpEndpoint src{};
        std::array<std::uint8_t, 128> buf{};
        auto res = listener.recvFrom(buf.data(), buf.size(), src);
        if (res.ok()) {
            auto pkt = InputPacket::decode(buf.data(), res.size);
            if (pkt) {
                receivedFlags.push_back(pkt->flags);
            }
        } else if (res.error == UdpError::WouldBlock || res.error == UdpError::Interrupted) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    sender.stop();
    ASSERT_EQ(receivedFlags.size(), 2u);
    EXPECT_EQ(receivedFlags[0], first.flags);
    EXPECT_EQ(receivedFlags[1], second.flags);
}

TEST(NetworkSender, SequenceIdTruncatedToUint16)
{
    UdpSocket listener;
    ASSERT_TRUE(listener.open(IpEndpoint::v4(0, 0, 0, 0, 0)));
    listener.setNonBlocking(true);
    auto listenEp = ensureLoopback(listener.localEndpoint());

    InputBuffer buffer;
    NetworkSender sender(buffer, listenEp, 3, std::chrono::milliseconds(5));
    ASSERT_TRUE(sender.start());

    InputCommand cmd{};
    cmd.flags      = InputMapper::UpFlag;
    cmd.sequenceId = 70000;
    buffer.push(cmd);

    bool received = false;
    auto start    = std::chrono::steady_clock::now();
    while (!received && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(300)) {
        IpEndpoint src{};
        std::array<std::uint8_t, 128> buf{};
        auto res = listener.recvFrom(buf.data(), buf.size(), src);
        if (res.ok()) {
            auto pkt = InputPacket::decode(buf.data(), res.size);
            if (pkt) {
                EXPECT_EQ(pkt->header.sequenceId, static_cast<std::uint16_t>(cmd.sequenceId & 0xFFFF));
                received = true;
                break;
            }
        } else if (res.error == UdpError::WouldBlock || res.error == UdpError::Interrupted) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    sender.stop();
    EXPECT_TRUE(received);
}

TEST(NetworkSender, StartTwiceReturnsFalse)
{
    InputBuffer buffer;
    NetworkSender sender(buffer, IpEndpoint::v4(127, 0, 0, 1, 60000), 1);
    ASSERT_TRUE(sender.start());
    EXPECT_FALSE(sender.start());
    sender.stop();
}

TEST(NetworkSender, StopIsIdempotent)
{
    InputBuffer buffer;
    NetworkSender sender(buffer, IpEndpoint::v4(127, 0, 0, 1, 60001), 1);
    ASSERT_TRUE(sender.start());
    sender.stop();
    sender.stop();
    SUCCEED();
}
