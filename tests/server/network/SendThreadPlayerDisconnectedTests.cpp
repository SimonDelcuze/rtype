#include "network/PlayerDisconnectedPacket.hpp"
#include "network/SendThread.hpp"

#include <array>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static bool recvPacket(UdpSocket& rx, PlayerDisconnectedPacket& out, int attempts = 200)
{
    std::array<std::uint8_t, PlayerDisconnectedPacket::kSize> buf{};
    IpEndpoint src{};
    for (int i = 0; i < attempts; ++i) {
        auto r = rx.recvFrom(buf.data(), buf.size(), src);
        if (r.ok()) {
            auto decoded = PlayerDisconnectedPacket::decode(buf.data(), r.size);
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

TEST(SendThread, DoesNotCrashWithoutPayload)
{
    std::vector<IpEndpoint> clients;
    SendThread send(IpEndpoint::v4(127, 0, 0, 1, 0), clients, 60.0);
    ASSERT_TRUE(send.start());
    PlayerDisconnectedPacket pkt{};
    pkt.playerId = 1;
    send.broadcast(pkt);
    send.stop();
}

TEST(SendThread, BroadcastsPlayerDisconnectedToAllClients)
{
    UdpSocket c1;
    UdpSocket c2;
    ASSERT_TRUE(c1.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    ASSERT_TRUE(c2.open(IpEndpoint::v4(127, 0, 0, 1, 0)));

    std::vector<IpEndpoint> clients{c1.localEndpoint(), c2.localEndpoint()};
    SendThread send(IpEndpoint::v4(127, 0, 0, 1, 0), clients, 60.0);
    ASSERT_TRUE(send.start());

    PlayerDisconnectedPacket pkt{};
    pkt.playerId          = 777;
    pkt.header.sequenceId = 9;
    send.broadcast(pkt);

    PlayerDisconnectedPacket got1{};
    PlayerDisconnectedPacket got2{};
    ASSERT_TRUE(recvPacket(c1, got1));
    ASSERT_TRUE(recvPacket(c2, got2));

    EXPECT_EQ(got1.playerId, pkt.playerId);
    EXPECT_EQ(got2.playerId, pkt.playerId);
    EXPECT_EQ(got1.header.sequenceId, pkt.header.sequenceId);
    EXPECT_EQ(got2.header.sequenceId, pkt.header.sequenceId);

    send.stop();
}

TEST(SendThread, BroadcastsWithUpdatedSequenceIds)
{
    UdpSocket c1;
    ASSERT_TRUE(c1.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    std::vector<IpEndpoint> clients{c1.localEndpoint()};
    SendThread send(IpEndpoint::v4(127, 0, 0, 1, 0), clients, 120.0);
    ASSERT_TRUE(send.start());

    PlayerDisconnectedPacket pkt{};
    pkt.playerId          = 101;
    pkt.header.sequenceId = 1;
    send.broadcast(pkt);

    PlayerDisconnectedPacket got{};
    ASSERT_TRUE(recvPacket(c1, got));
    EXPECT_EQ(got.header.sequenceId, pkt.header.sequenceId);

    pkt.header.sequenceId = 2;
    pkt.playerId          = 202;
    send.broadcast(pkt);
    ASSERT_TRUE(recvPacket(c1, got));
    EXPECT_EQ(got.header.sequenceId, pkt.header.sequenceId);
    EXPECT_EQ(got.playerId, pkt.playerId);

    send.stop();
}
