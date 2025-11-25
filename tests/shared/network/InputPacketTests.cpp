#include "network/InputPacket.hpp"

#include <array>
#include <gtest/gtest.h>
#include <limits>
#include <vector>

TEST(InputPacket, EncodeDecodeRoundTrip)
{
    InputPacket p{};
    p.header.sequenceId = 0x1234;
    p.header.tickId     = 0x0A0B0C0D;
    p.playerId          = 0x01020304;
    p.flags             = static_cast<std::uint16_t>(InputFlag::MoveUp | InputFlag::Fire);
    p.x                 = 1.5F;
    p.y                 = -2.25F;
    p.angle             = 3.125F;

    auto buf = p.encode();
    EXPECT_EQ(buf.size(), InputPacket::kSize);

    auto decoded = InputPacket::decode(buf.data(), buf.size());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->header.version, PacketHeader::kProtocolVersion);
    EXPECT_EQ(decoded->header.packetType, static_cast<std::uint8_t>(PacketType::ClientToServer));
    EXPECT_EQ(decoded->header.messageType, static_cast<std::uint8_t>(MessageType::Input));
    EXPECT_EQ(decoded->header.payloadSize, InputPacket::kPayloadSize);
    EXPECT_EQ(decoded->header.sequenceId, p.header.sequenceId);
    EXPECT_EQ(decoded->header.tickId, p.header.tickId);
    EXPECT_EQ(decoded->playerId, p.playerId);
    EXPECT_EQ(decoded->flags, p.flags);
    EXPECT_FLOAT_EQ(decoded->x, p.x);
    EXPECT_FLOAT_EQ(decoded->y, p.y);
    EXPECT_FLOAT_EQ(decoded->angle, p.angle);
}

TEST(InputPacket, RejectWrongSize)
{
    std::array<std::uint8_t, InputPacket::kSize - 1> buf{};
    auto decoded = InputPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(InputPacket, RejectWrongType)
{
    InputPacket p{};
    auto buf = p.encode();
    // messageType is at offset 6 in the 15-byte header
    buf[6] = static_cast<std::uint8_t>(MessageType::Snapshot);
    auto decoded = InputPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(InputPacket, RejectNonFinite)
{
    InputPacket p{};
    p.angle      = std::numeric_limits<float>::infinity();
    auto buf     = p.encode();
    auto decoded = InputPacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}
