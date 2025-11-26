#include "network/EntitySpawnPacket.hpp"

#include <gtest/gtest.h>
#include <vector>

TEST(EntitySpawnPacket, EncodeDecodeRoundtrip)
{
    EntitySpawnPacket p{};
    p.header.sequenceId = 3;
    p.header.tickId     = 11;
    p.entityId          = 99;
    p.entityType        = 5;
    p.posX              = -12.5F;
    p.posY              = 42.0F;
    auto buf            = p.encode();

    auto decoded = EntitySpawnPacket::decode(buf.data(), buf.size());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->header.messageType, static_cast<std::uint8_t>(MessageType::EntitySpawn));
    EXPECT_EQ(decoded->header.packetType, static_cast<std::uint8_t>(PacketType::ServerToClient));
    EXPECT_EQ(decoded->header.payloadSize, EntitySpawnPacket::kPayloadSize);
    EXPECT_EQ(decoded->entityId, p.entityId);
    EXPECT_EQ(decoded->entityType, p.entityType);
    EXPECT_FLOAT_EQ(decoded->posX, p.posX);
    EXPECT_FLOAT_EQ(decoded->posY, p.posY);
}

TEST(EntitySpawnPacket, RejectWrongType)
{
    EntitySpawnPacket p{};
    auto buf     = p.encode();
    buf[6]       = static_cast<std::uint8_t>(MessageType::Snapshot);
    auto decoded = EntitySpawnPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(EntitySpawnPacket, RejectWrongPacketDirection)
{
    EntitySpawnPacket p{};
    auto buf     = p.encode();
    buf[5]       = static_cast<std::uint8_t>(PacketType::ClientToServer);
    auto decoded = EntitySpawnPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(EntitySpawnPacket, RejectWrongSize)
{
    std::vector<std::uint8_t> buf(EntitySpawnPacket::kSize - 1, 0);
    auto decoded = EntitySpawnPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(EntitySpawnPacket, RejectCrcMismatch)
{
    EntitySpawnPacket p{};
    auto buf = p.encode();
    buf.back() ^= 0xFF;
    auto decoded = EntitySpawnPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(EntitySpawnPacket, RejectNonFinitePosition)
{
    EntitySpawnPacket p{};
    p.posX       = std::numeric_limits<float>::infinity();
    auto buf     = p.encode();
    auto decoded = EntitySpawnPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}
