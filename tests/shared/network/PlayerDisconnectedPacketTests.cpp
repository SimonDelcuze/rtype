#include "network/PlayerDisconnectedPacket.hpp"

#include <gtest/gtest.h>
#include <vector>

TEST(PlayerDisconnectedPacket, EncodeDecodeRoundtrip)
{
    PlayerDisconnectedPacket p{};
    p.header.sequenceId = 42;
    p.header.tickId     = 99;
    p.playerId          = 1234;
    auto buf            = p.encode();

    auto decoded = PlayerDisconnectedPacket::decode(buf.data(), buf.size());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->header.messageType, static_cast<std::uint8_t>(MessageType::PlayerDisconnected));
    EXPECT_EQ(decoded->header.packetType, static_cast<std::uint8_t>(PacketType::ServerToClient));
    EXPECT_EQ(decoded->header.payloadSize, PlayerDisconnectedPacket::kPayloadSize);
    EXPECT_EQ(decoded->header.sequenceId, p.header.sequenceId);
    EXPECT_EQ(decoded->header.tickId, p.header.tickId);
    EXPECT_EQ(decoded->playerId, p.playerId);
}

TEST(PlayerDisconnectedPacket, RejectWrongType)
{
    PlayerDisconnectedPacket p{};
    auto buf     = p.encode();
    buf[6]       = static_cast<std::uint8_t>(MessageType::Snapshot);
    auto decoded = PlayerDisconnectedPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(PlayerDisconnectedPacket, RejectWrongPacketDirection)
{
    PlayerDisconnectedPacket p{};
    auto buf     = p.encode();
    buf[5]       = static_cast<std::uint8_t>(PacketType::ClientToServer);
    auto decoded = PlayerDisconnectedPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(PlayerDisconnectedPacket, RejectWrongSize)
{
    std::vector<std::uint8_t> buf(PlayerDisconnectedPacket::kSize - 1, 0);
    auto decoded = PlayerDisconnectedPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(PlayerDisconnectedPacket, RejectCrcMismatch)
{
    PlayerDisconnectedPacket p{};
    auto buf = p.encode();
    buf.back() ^= 0xFF;
    auto decoded = PlayerDisconnectedPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(PlayerDisconnectedPacket, RejectCorruptedPayload)
{
    PlayerDisconnectedPacket p{};
    p.playerId = 555;
    auto buf   = p.encode();
    buf[PacketHeader::kSize] ^= 0xFF;
    auto decoded = PlayerDisconnectedPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}
