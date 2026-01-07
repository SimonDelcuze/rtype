#include "network/InputReceiveThread.hpp"
#include "network/SendThread.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace std::chrono_literals;

static bool recvExact(UdpSocket& rx, std::vector<std::uint8_t>& out, int attempts = 200)
{
    std::array<std::uint8_t, 64> buf{};
    IpEndpoint src{};
    for (int i = 0; i < attempts; ++i) {
        auto r = rx.recvFrom(buf.data(), buf.size(), src);
        if (r.ok() && r.size > 0) {
            out.assign(buf.begin(), buf.begin() + r.size);
            return true;
        } else if (r.error == UdpError::WouldBlock) {
            std::this_thread::sleep_for(1ms);
            continue;
        }
    }
    return false;
}

TEST(MultipleClientsIntegration, SendThreadBroadcastsToManyClients)
{
    UdpSocket c1;
    UdpSocket c2;
    UdpSocket c3;
    ASSERT_TRUE(c1.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    ASSERT_TRUE(c2.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    ASSERT_TRUE(c3.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    std::vector<IpEndpoint> clients{c1.localEndpoint(), c2.localEndpoint(), c3.localEndpoint()};
    SendThread send(IpEndpoint::v4(127, 0, 0, 1, 0), clients, 120.0);
    ASSERT_TRUE(send.start());

    DeltaStatePacket pkt{};
    pkt.header.sequenceId = 10;
    pkt.header.tickId     = 3;
    pkt.entries.push_back({1, 0.0F, 0.0F, 0.0F, 0.0F, 1});
    send.publish(pkt);
    std::this_thread::sleep_for(20ms);

    std::vector<std::uint8_t> b1;
    std::vector<std::uint8_t> b2;
    std::vector<std::uint8_t> b3;
    ASSERT_TRUE(recvExact(c1, b1));
    ASSERT_TRUE(recvExact(c2, b2));
    ASSERT_TRUE(recvExact(c3, b3));
    EXPECT_EQ(b1, b2);
    EXPECT_EQ(b2, b3);

    send.stop();
}

TEST(MultipleClientsIntegration, ReceiveThreadMaintainsIndependentSequence)
{
    ThreadSafeQueue<ReceivedInput> queue;
    InputReceiveThread rx(IpEndpoint::v4(127, 0, 0, 1, 0), queue);
    ASSERT_TRUE(rx.start());
    auto ep = rx.endpoint();
    ASSERT_NE(ep.port, 0);

    UdpSocket c1;
    UdpSocket c2;
    ASSERT_TRUE(c1.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    ASSERT_TRUE(c2.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    InputPacket p1{};
    p1.header.sequenceId = 1;
    p1.playerId          = 1;
    auto buf1            = p1.encode();
    ASSERT_TRUE(c1.sendTo(buf1.data(), buf1.size(), ep).ok());

    InputPacket p2{};
    p2.header.sequenceId = 5;
    p2.playerId          = 2;
    auto buf2            = p2.encode();
    ASSERT_TRUE(c2.sendTo(buf2.data(), buf2.size(), ep).ok());

    ReceivedInput ri{};
    int popped = 0;
    for (int i = 0; i < 200; ++i) {
        if (queue.tryPop(ri))
            ++popped;
        std::this_thread::sleep_for(1ms);
    }
    EXPECT_EQ(popped, 2);

    auto s1 = rx.clientState(c1.localEndpoint());
    auto s2 = rx.clientState(c2.localEndpoint());
    ASSERT_TRUE(s1.has_value());
    ASSERT_TRUE(s2.has_value());
    EXPECT_EQ(s1->lastSequenceId, p1.header.sequenceId);
    EXPECT_EQ(s2->lastSequenceId, p2.header.sequenceId);

    rx.stop();
}
