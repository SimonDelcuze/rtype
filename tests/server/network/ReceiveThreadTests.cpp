#include "network/InputReceiveThread.hpp"

#include <chrono>
#include <cmath>
#include <gtest/gtest.h>
#include <thread>

using namespace std::chrono_literals;

static bool pollPop(ThreadSafeQueue<ReceivedInput>& q, ReceivedInput& out, int attempts = 200)
{
    for (int i = 0; i < attempts; ++i) {
        if (q.tryPop(out))
            return true;
        std::this_thread::sleep_for(1ms);
    }
    return false;
}

TEST(InputReceiveThread, EnqueueValidInput)
{
    ThreadSafeQueue<ReceivedInput> queue;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue);
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket p{};
    p.header.sequenceId = 1;
    p.header.tickId     = 10;
    p.playerId          = 5;
    p.flags             = static_cast<std::uint16_t>(InputFlag::MoveLeft);
    p.x                 = 2.0F;
    p.y                 = 3.0F;
    p.angle             = 0.5F;
    auto buf            = p.encode();

    auto sr = tx.sendTo(buf.data(), buf.size(), ep);
    ASSERT_TRUE(sr.ok());

    ReceivedInput got{};
    ASSERT_TRUE(pollPop(queue, got));
    EXPECT_EQ(got.input.playerId, p.playerId);
    EXPECT_EQ(got.input.flags, p.flags);
    EXPECT_EQ(got.input.sequenceId, p.header.sequenceId);
    EXPECT_EQ(got.input.tickId, p.header.tickId);
    EXPECT_FLOAT_EQ(got.input.x, p.x);
    EXPECT_FLOAT_EQ(got.input.y, p.y);
    EXPECT_FLOAT_EQ(got.input.angle, p.angle);

    rt.stop();
}

TEST(InputReceiveThread, IgnoreInvalidType)
{
    ThreadSafeQueue<ReceivedInput> queue;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue);
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket p{};
    auto buf = p.encode();
    buf[0]   = static_cast<std::uint8_t>(MessageType::Snapshot);
    auto sr  = tx.sendTo(buf.data(), buf.size(), ep);
    ASSERT_TRUE(sr.ok());

    ReceivedInput got{};
    EXPECT_FALSE(pollPop(queue, got, 100));

    rt.stop();
}

TEST(InputReceiveThread, DropStaleSequence)
{
    ThreadSafeQueue<ReceivedInput> queue;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue);
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket newer{};
    newer.header.sequenceId = 2;
    newer.playerId          = 9;
    auto bufNew             = newer.encode();
    ASSERT_TRUE(tx.sendTo(bufNew.data(), bufNew.size(), ep).ok());

    InputPacket older{};
    older.header.sequenceId = 1;
    older.playerId          = 9;
    auto bufOld             = older.encode();
    ASSERT_TRUE(tx.sendTo(bufOld.data(), bufOld.size(), ep).ok());

    ReceivedInput got{};
    ASSERT_TRUE(pollPop(queue, got));
    EXPECT_EQ(got.input.sequenceId, newer.header.sequenceId);
    EXPECT_FALSE(pollPop(queue, got, 50));

    rt.stop();
}

TEST(InputReceiveThread, RejectInvalidFlags)
{
    ThreadSafeQueue<ReceivedInput> queue;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue);
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket p{};
    p.flags  = 0xFFFF;
    auto buf = p.encode();
    ASSERT_TRUE(tx.sendTo(buf.data(), buf.size(), ep).ok());

    ReceivedInput got{};
    EXPECT_FALSE(pollPop(queue, got, 100));

    rt.stop();
}

TEST(InputReceiveThread, RejectNonFinite)
{
    ThreadSafeQueue<ReceivedInput> queue;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue);
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket p{};
    p.angle  = std::numeric_limits<float>::quiet_NaN();
    auto buf = p.encode();
    ASSERT_TRUE(tx.sendTo(buf.data(), buf.size(), ep).ok());

    ReceivedInput got{};
    EXPECT_FALSE(pollPop(queue, got, 100));

    rt.stop();
}

TEST(InputReceiveThread, RejectWrongSizePacket)
{
    ThreadSafeQueue<ReceivedInput> queue;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue);
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    PacketHeader h{};
    h.messageType = static_cast<std::uint8_t>(MessageType::Input);
    auto hdr      = h.encode();
    ASSERT_TRUE(tx.sendTo(hdr.data(), hdr.size(), ep).ok());

    ReceivedInput got{};
    EXPECT_FALSE(pollPop(queue, got, 100));

    rt.stop();
}

TEST(InputReceiveThread, SeparateSequencePerEndpoint)
{
    ThreadSafeQueue<ReceivedInput> queue;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue);
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket txA;
    ASSERT_TRUE(txA.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    UdpSocket txB;
    ASSERT_TRUE(txB.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket a{};
    a.header.sequenceId = 1;
    a.playerId          = 1;
    auto bufA           = a.encode();
    ASSERT_TRUE(txA.sendTo(bufA.data(), bufA.size(), ep).ok());

    InputPacket b{};
    b.header.sequenceId = 1;
    b.playerId          = 2;
    auto bufB           = b.encode();
    ASSERT_TRUE(txB.sendTo(bufB.data(), bufB.size(), ep).ok());

    ReceivedInput got{};
    ASSERT_TRUE(pollPop(queue, got));
    ASSERT_TRUE(pollPop(queue, got));

    rt.stop();
}
