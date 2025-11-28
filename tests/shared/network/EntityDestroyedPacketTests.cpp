#include "network/EntityDestroyedPacket.hpp"

#include <gtest/gtest.h>
#include <vector>

TEST(EntityDestroyedPacket, EncodeDecodeRoundtrip)
{
    EntityDestroyedPacket p{};
    p.header.sequenceId = 8;
    p.header.tickId     = 77;
    p.entityId          = 555;
    auto buf            = p.encode();

    auto decoded = EntityDestroyedPacket::decode(buf.data(), buf.size());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->header.messageType, static_cast<std::uint8_t>(MessageType::EntityDestroyed));
    EXPECT_EQ(decoded->header.packetType, static_cast<std::uint8_t>(PacketType::ServerToClient));
    EXPECT_EQ(decoded->header.payloadSize, EntityDestroyedPacket::kPayloadSize);
    EXPECT_EQ(decoded->header.sequenceId, p.header.sequenceId);
    EXPECT_EQ(decoded->header.tickId, p.header.tickId);
    EXPECT_EQ(decoded->entityId, p.entityId);
}

TEST(EntityDestroyedPacket, RejectWrongType)
{
    EntityDestroyedPacket p{};
    auto buf     = p.encode();
    buf[6]       = static_cast<std::uint8_t>(MessageType::Snapshot);
    auto decoded = EntityDestroyedPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(EntityDestroyedPacket, RejectWrongDirection)
{
    EntityDestroyedPacket p{};
    auto buf     = p.encode();
    buf[5]       = static_cast<std::uint8_t>(PacketType::ClientToServer);
    auto decoded = EntityDestroyedPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(EntityDestroyedPacket, RejectWrongSize)
{
    std::vector<std::uint8_t> buf(EntityDestroyedPacket::kSize - 1, 0);
    auto decoded = EntityDestroyedPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(EntityDestroyedPacket, RejectCrcMismatch)
{
    EntityDestroyedPacket p{};
    auto buf = p.encode();
    buf.back() ^= 0xFF;
    auto decoded = EntityDestroyedPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(EntityDestroyedPacket, RejectCorruptedPayload)
{
    EntityDestroyedPacket p{};
    p.entityId = 123;
    auto buf   = p.encode();
    buf[PacketHeader::kSize] ^= 0xFF;
    auto decoded = EntityDestroyedPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}
