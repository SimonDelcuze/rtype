#include "network/EntityDestroyedPacket.hpp"
#include "network/SendThread.hpp"

#include <array>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static bool recvDestroyed(UdpSocket& rx, EntityDestroyedPacket& out, int attempts = 200)
{
    std::array<std::uint8_t, EntityDestroyedPacket::kSize> buf{};
    IpEndpoint src{};
    for (int i = 0; i < attempts; ++i) {
        auto r = rx.recvFrom(buf.data(), buf.size(), src);
        if (r.ok()) {
            auto decoded = EntityDestroyedPacket::decode(buf.data(), r.size);
            if (decoded) {
                out = *decoded;
                return true;
            }
        } else if (r.error == UdpError::WouldBlock) {
            std::this_thread::sleep_for(1ms);
            continue;
        }
    }
    return false;
}

TEST(SendThread, BroadcastsEntityDestroyedToAllClients)
{
    UdpSocket c1;
    UdpSocket c2;
    ASSERT_TRUE(c1.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    ASSERT_TRUE(c2.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    std::vector<IpEndpoint> clients{c1.localEndpoint(), c2.localEndpoint()};
    SendThread send(IpEndpoint::v4(127, 0, 0, 1, 0), clients, 60.0);
    ASSERT_TRUE(send.start());

    EntityDestroyedPacket pkt{};
    pkt.entityId          = 999;
    pkt.header.sequenceId = 4;
    send.broadcast(pkt);

    EntityDestroyedPacket got1{};
    EntityDestroyedPacket got2{};
    ASSERT_TRUE(recvDestroyed(c1, got1));
    ASSERT_TRUE(recvDestroyed(c2, got2));

    EXPECT_EQ(got1.entityId, pkt.entityId);
    EXPECT_EQ(got2.entityId, pkt.entityId);
    EXPECT_EQ(got1.header.sequenceId, pkt.header.sequenceId);
    EXPECT_EQ(got2.header.sequenceId, pkt.header.sequenceId);

    send.stop();
}

TEST(SendThread, DestroyedSequenceUpdates)
{
    UdpSocket c1;
    ASSERT_TRUE(c1.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    std::vector<IpEndpoint> clients{c1.localEndpoint()};
    SendThread send(IpEndpoint::v4(127, 0, 0, 1, 0), clients, 120.0);
    ASSERT_TRUE(send.start());

    EntityDestroyedPacket pkt{};
    pkt.entityId          = 1;
    pkt.header.sequenceId = 10;
    send.broadcast(pkt);
    EntityDestroyedPacket got{};
    ASSERT_TRUE(recvDestroyed(c1, got));
    EXPECT_EQ(got.header.sequenceId, pkt.header.sequenceId);
    EXPECT_EQ(got.entityId, pkt.entityId);

    pkt.entityId          = 2;
    pkt.header.sequenceId = 11;
    send.broadcast(pkt);
    ASSERT_TRUE(recvDestroyed(c1, got));
    EXPECT_EQ(got.header.sequenceId, pkt.header.sequenceId);
    EXPECT_EQ(got.entityId, pkt.entityId);

    send.stop();
}

TEST(SendThread, DestroyedBroadcastNoClients)
{
    std::vector<IpEndpoint> clients;
    SendThread send(IpEndpoint::v4(127, 0, 0, 1, 0), clients, 60.0);
    ASSERT_TRUE(send.start());
    EntityDestroyedPacket pkt{};
    pkt.entityId = 42;
    send.broadcast(pkt);
    send.stop();
}
