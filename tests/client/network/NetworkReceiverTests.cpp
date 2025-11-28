#include "network/NetworkReceiver.hpp"
#include "network/PacketHeader.hpp"
#include "network/UdpSocket.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
    std::vector<std::uint8_t> makeSnapshotPacket(std::uint16_t sequenceId = 1, std::uint32_t tickId = 42,
                                                 bool withCrc = true)
    {
        PacketHeader h{};
        h.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        h.messageType = static_cast<std::uint8_t>(MessageType::Snapshot);
        h.sequenceId  = sequenceId;
        h.tickId      = tickId;
        h.payloadSize = 2;
        auto hdr      = h.encode();
        std::vector<std::uint8_t> buf(hdr.begin(), hdr.end());
        buf.push_back(0);
        buf.push_back(0);
        if (withCrc) {
            std::uint32_t crc = PacketHeader::crc32(buf.data(), buf.size());
            buf.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
            buf.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
            buf.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
            buf.push_back(static_cast<std::uint8_t>(crc & 0xFF));
        }
        return buf;
    }

    std::vector<std::uint8_t> makeNonSnapshotPacket()
    {
        PacketHeader h{};
        h.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        h.messageType = static_cast<std::uint8_t>(MessageType::Input);
        h.sequenceId  = 1;
        h.tickId      = 1;
        h.payloadSize = 0;
        auto hdr      = h.encode();
        return std::vector<std::uint8_t>(hdr.begin(), hdr.end());
    }

    std::vector<std::uint8_t> makeClientToServerPacket()
    {
        PacketHeader h{};
        h.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
        h.messageType = static_cast<std::uint8_t>(MessageType::Snapshot);
        h.sequenceId  = 1;
        h.tickId      = 1;
        h.payloadSize = 0;
        auto hdr      = h.encode();
        return std::vector<std::uint8_t>(hdr.begin(), hdr.end());
    }

    bool sendPacket(const std::vector<std::uint8_t>& data, const IpEndpoint& dst)
    {
        IpEndpoint target = dst;
        if (target.addr == std::array<std::uint8_t, 4>{0, 0, 0, 0})
            target.addr = {127, 0, 0, 1};
        UdpSocket sender;
        if (!sender.open(IpEndpoint::v4(0, 0, 0, 0, 0))) {
            return false;
        }
        auto res = sender.sendTo(data.data(), data.size(), target);
        return res.ok();
    }
} // namespace

TEST(NetworkReceiver, ReceivesSnapshotPacketWithCrc)
{
    std::atomic<int> count{0};
    std::mutex m;
    std::condition_variable cv;

    NetworkReceiver rx(IpEndpoint::v4(0, 0, 0, 0, 0), [&](std::vector<std::uint8_t>&& pkt) {
        (void) pkt;
        ++count;
        cv.notify_one();
    });

    ASSERT_TRUE(rx.start());
    IpEndpoint ep = rx.endpoint();
    ASSERT_NE(ep.port, 0);

    auto data = makeSnapshotPacket();
    ASSERT_TRUE(sendPacket(data, ep));

    std::unique_lock<std::mutex> lock(m);
    cv.wait_for(lock, std::chrono::milliseconds(500), [&] { return count.load() > 0; });
    EXPECT_GT(count.load(), 0);

    rx.stop();
}

TEST(NetworkReceiver, IgnoresNonSnapshotPacket)
{
    std::atomic<int> count{0};

    NetworkReceiver rx(IpEndpoint::v4(0, 0, 0, 0, 0), [&](std::vector<std::uint8_t>&& pkt) {
        (void) pkt;
        ++count;
    });

    ASSERT_TRUE(rx.start());
    IpEndpoint ep = rx.endpoint();
    ASSERT_NE(ep.port, 0);

    auto buf = makeNonSnapshotPacket();
    ASSERT_TRUE(sendPacket(buf, ep));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(count.load(), 0);

    rx.stop();
}

TEST(NetworkReceiver, ReceivesSnapshotWithoutCrc)
{
    std::atomic<int> count{0};
    std::mutex m;
    std::condition_variable cv;

    NetworkReceiver rx(IpEndpoint::v4(0, 0, 0, 0, 0), [&](std::vector<std::uint8_t>&& pkt) {
        (void) pkt;
        ++count;
        cv.notify_one();
    });

    ASSERT_TRUE(rx.start());
    IpEndpoint ep = rx.endpoint();
    ASSERT_NE(ep.port, 0);

    auto data = makeSnapshotPacket(1, 1, false);
    ASSERT_TRUE(sendPacket(data, ep));

    std::unique_lock<std::mutex> lock(m);
    cv.wait_for(lock, std::chrono::milliseconds(500), [&] { return count.load() > 0; });
    EXPECT_GT(count.load(), 0);

    rx.stop();
}

TEST(NetworkReceiver, IgnoresClientToServerPacket)
{
    std::atomic<int> count{0};
    NetworkReceiver rx(IpEndpoint::v4(0, 0, 0, 0, 0), [&](std::vector<std::uint8_t>&& pkt) {
        (void) pkt;
        ++count;
    });
    ASSERT_TRUE(rx.start());
    IpEndpoint ep = rx.endpoint();
    ASSERT_NE(ep.port, 0);

    auto data = makeClientToServerPacket();
    ASSERT_TRUE(sendPacket(data, ep));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(count.load(), 0);

    rx.stop();
}

TEST(NetworkReceiver, IgnoresInvalidMagic)
{
    std::atomic<int> count{0};
    NetworkReceiver rx(IpEndpoint::v4(0, 0, 0, 0, 0), [&](std::vector<std::uint8_t>&& pkt) {
        (void) pkt;
        ++count;
    });
    ASSERT_TRUE(rx.start());
    IpEndpoint ep = rx.endpoint();
    ASSERT_NE(ep.port, 0);

    auto data = makeSnapshotPacket();
    data[0] ^= 0xFF;
    ASSERT_TRUE(sendPacket(data, ep));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(count.load(), 0);

    rx.stop();
}

TEST(NetworkReceiver, IgnoresInvalidVersion)
{
    std::atomic<int> count{0};
    NetworkReceiver rx(IpEndpoint::v4(0, 0, 0, 0, 0), [&](std::vector<std::uint8_t>&& pkt) {
        (void) pkt;
        ++count;
    });
    ASSERT_TRUE(rx.start());
    IpEndpoint ep = rx.endpoint();
    ASSERT_NE(ep.port, 0);

    auto data = makeSnapshotPacket();
    data[4]   = static_cast<std::uint8_t>(PacketHeader::kProtocolVersion + 1);
    ASSERT_TRUE(sendPacket(data, ep));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(count.load(), 0);

    rx.stop();
}

TEST(NetworkReceiver, IgnoresTruncatedHeader)
{
    std::atomic<int> count{0};
    NetworkReceiver rx(IpEndpoint::v4(0, 0, 0, 0, 0), [&](std::vector<std::uint8_t>&& pkt) {
        (void) pkt;
        ++count;
    });
    ASSERT_TRUE(rx.start());
    IpEndpoint ep = rx.endpoint();
    ASSERT_NE(ep.port, 0);

    auto data = makeSnapshotPacket();
    data.resize(5);
    ASSERT_TRUE(sendPacket(data, ep));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(count.load(), 0);

    rx.stop();
}

TEST(NetworkReceiver, IgnoresPayloadSmallerThanDeclared)
{
    std::atomic<int> count{0};
    NetworkReceiver rx(IpEndpoint::v4(0, 0, 0, 0, 0), [&](std::vector<std::uint8_t>&& pkt) {
        (void) pkt;
        ++count;
    });
    ASSERT_TRUE(rx.start());
    IpEndpoint ep = rx.endpoint();
    ASSERT_NE(ep.port, 0);

    PacketHeader h{};
    h.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    h.messageType = static_cast<std::uint8_t>(MessageType::Snapshot);
    h.payloadSize = 10;
    h.sequenceId  = 1;
    h.tickId      = 1;
    auto hdr      = h.encode();
    std::vector<std::uint8_t> data(hdr.begin(), hdr.end());

    ASSERT_TRUE(sendPacket(data, ep));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(count.load(), 0);

    rx.stop();
}

TEST(NetworkReceiver, TrimsTrailingGarbage)
{
    std::mutex m;
    std::condition_variable cv;
    std::size_t recordedSize = 0;

    NetworkReceiver rx(IpEndpoint::v4(0, 0, 0, 0, 0), [&](std::vector<std::uint8_t>&& pkt) {
        recordedSize = pkt.size();
        cv.notify_one();
    });
    ASSERT_TRUE(rx.start());
    IpEndpoint ep = rx.endpoint();
    ASSERT_NE(ep.port, 0);

    auto data = makeSnapshotPacket();
    data.resize(data.size() + 5, 0xAA);
    ASSERT_TRUE(sendPacket(data, ep));

    std::unique_lock<std::mutex> lock(m);
    cv.wait_for(lock, std::chrono::milliseconds(500), [&] { return recordedSize > 0; });

    auto expected = PacketHeader::kSize + 2 + PacketHeader::kCrcSize;
    EXPECT_EQ(recordedSize, expected);

    rx.stop();
}

TEST(NetworkReceiver, ReceivesMultiplePackets)
{
    std::atomic<int> count{0};
    std::mutex m;
    std::condition_variable cv;

    NetworkReceiver rx(IpEndpoint::v4(0, 0, 0, 0, 0), [&](std::vector<std::uint8_t>&& pkt) {
        (void) pkt;
        ++count;
        cv.notify_one();
    });
    ASSERT_TRUE(rx.start());
    IpEndpoint ep = rx.endpoint();
    ASSERT_NE(ep.port, 0);

    for (int i = 0; i < 3; ++i) {
        auto data = makeSnapshotPacket(static_cast<std::uint16_t>(i + 1), static_cast<std::uint32_t>(i + 1));
        ASSERT_TRUE(sendPacket(data, ep));
    }

    std::unique_lock<std::mutex> lock(m);
    cv.wait_for(lock, std::chrono::milliseconds(500), [&] { return count.load() >= 3; });
    EXPECT_GE(count.load(), 3);

    rx.stop();
}
