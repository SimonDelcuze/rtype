#include "events/ClientTimeoutEvent.hpp"
#include "network/InputReceiveThread.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <vector>

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

static bool pollTimeout(ThreadSafeQueue<ClientTimeoutEvent>& q, ClientTimeoutEvent& out, int attempts = 200)
{
    for (int i = 0; i < attempts; ++i) {
        if (q.tryPop(out))
            return true;
        std::this_thread::sleep_for(1ms);
    }
    return false;
}

static std::streampos getLogSize()
{
    std::ifstream f("logs/server.log", std::ios::binary | std::ios::ate);
    if (!f.is_open())
        return 0;
    return f.tellg();
}

static std::string readLogFileFrom(std::streampos startPos)
{
    std::ifstream f("logs/server.log", std::ios::binary);
    if (!f.is_open())
        return {};
    f.seekg(startPos);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
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

TEST(InputReceiveThread, ClientStateStoredOnValidPacket)
{
    ThreadSafeQueue<ReceivedInput> queue;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue);
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket p{};
    p.header.sequenceId = 3;
    p.header.tickId     = 20;
    auto buf            = p.encode();
    ASSERT_TRUE(tx.sendTo(buf.data(), buf.size(), ep).ok());

    ReceivedInput got{};
    ASSERT_TRUE(pollPop(queue, got));

    std::optional<ClientState> state;
    for (int i = 0; i < 50; ++i) {
        state = rt.clientState(got.from);
        if (state)
            break;
        std::this_thread::sleep_for(1ms);
    }
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state->lastSequenceId, p.header.sequenceId);
    auto now = std::chrono::steady_clock::now();
    auto dt  = now - state->lastPacketTime;
    EXPECT_LT(dt, 1s);

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

    auto state = rt.clientState(ep);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state->lastSequenceId, newer.header.sequenceId);

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

    auto stateA = rt.clientState(txA.localEndpoint());
    auto stateB = rt.clientState(txB.localEndpoint());
    ASSERT_TRUE(stateA.has_value());
    ASSERT_TRUE(stateB.has_value());
    EXPECT_EQ(stateA->lastSequenceId, a.header.sequenceId);
    EXPECT_EQ(stateB->lastSequenceId, b.header.sequenceId);
    EXPECT_NE(stateA->lastPacketTime, stateB->lastPacketTime);

    rt.stop();
}

TEST(InputReceiveThread, StaleDoesNotUpdateState)
{
    ThreadSafeQueue<ReceivedInput> queue;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue);
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket first{};
    first.header.sequenceId = 4;
    auto bufFirst           = first.encode();
    ASSERT_TRUE(tx.sendTo(bufFirst.data(), bufFirst.size(), ep).ok());
    ReceivedInput got{};
    ASSERT_TRUE(pollPop(queue, got));
    auto state1 = rt.clientState(tx.localEndpoint());
    ASSERT_TRUE(state1.has_value());

    std::this_thread::sleep_for(5ms);

    InputPacket stale{};
    stale.header.sequenceId = 3;
    auto bufStale           = stale.encode();
    ASSERT_TRUE(tx.sendTo(bufStale.data(), bufStale.size(), ep).ok());
    EXPECT_FALSE(pollPop(queue, got, 50));

    auto state2 = rt.clientState(tx.localEndpoint());
    ASSERT_TRUE(state2.has_value());
    EXPECT_EQ(state2->lastSequenceId, state1->lastSequenceId);
    EXPECT_EQ(state2->lastPacketTime, state1->lastPacketTime);

    rt.stop();
}









TEST(InputReceiveThread, TimeoutEventEmittedAfterSilence)
{
    ThreadSafeQueue<ReceivedInput> queue;
    ThreadSafeQueue<ClientTimeoutEvent> timeouts;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue, &timeouts, std::chrono::milliseconds(30));
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket p{};
    p.header.sequenceId = 7;
    auto buf            = p.encode();
    ASSERT_TRUE(tx.sendTo(buf.data(), buf.size(), ep).ok());

    ReceivedInput got{};
    ASSERT_TRUE(pollPop(queue, got));

    ClientTimeoutEvent ev{};
    ASSERT_TRUE(pollTimeout(timeouts, ev, 500));
    EXPECT_EQ(ev.endpoint.port, tx.localEndpoint().port);
    EXPECT_EQ(ev.lastSequenceId, p.header.sequenceId);

    rt.stop();
}

TEST(InputReceiveThread, TimeoutNotEmittedBeforeThreshold)
{
    ThreadSafeQueue<ReceivedInput> queue;
    ThreadSafeQueue<ClientTimeoutEvent> timeouts;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue, &timeouts, std::chrono::milliseconds(100));
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket p{};
    p.header.sequenceId = 2;
    auto buf            = p.encode();
    ASSERT_TRUE(tx.sendTo(buf.data(), buf.size(), ep).ok());

    ReceivedInput got{};
    ASSERT_TRUE(pollPop(queue, got));

    ClientTimeoutEvent ev{};
    EXPECT_FALSE(pollTimeout(timeouts, ev, 50));
    EXPECT_TRUE(pollTimeout(timeouts, ev, 200));

    rt.stop();
}



TEST(InputReceiveThread, TimeoutNotRepeatedAfterEvent)
{
    ThreadSafeQueue<ReceivedInput> queue;
    ThreadSafeQueue<ClientTimeoutEvent> timeouts;
    InputReceiveThread rt(IpEndpoint::v4(127, 0, 0, 1, 0), queue, &timeouts, std::chrono::milliseconds(50));
    ASSERT_TRUE(rt.start());
    auto ep = rt.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket p{};
    p.header.sequenceId = 4;
    auto buf            = p.encode();
    ASSERT_TRUE(tx.sendTo(buf.data(), buf.size(), ep).ok());
    ReceivedInput got{};
    ASSERT_TRUE(pollPop(queue, got));

    ClientTimeoutEvent ev{};
    ASSERT_TRUE(pollTimeout(timeouts, ev, 200));
    EXPECT_FALSE(pollTimeout(timeouts, ev, 200));

    rt.stop();
}


