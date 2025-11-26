#include "network/InputParser.hpp"

#include <array>
#include <gtest/gtest.h>

TEST(InputParser, ParseValidPacket)
{
    InputPacket p{};
    p.header.sequenceId = 0x42;
    p.header.tickId     = 0x01020304;
    p.playerId          = 7;
    p.flags             = static_cast<std::uint16_t>(InputFlag::MoveUp | InputFlag::Fire);
    p.x                 = 10.0F;
    p.y                 = -5.5F;
    p.angle             = 1.25F;
    auto buf            = p.encode();

    auto decoded = parseInputPacket(buf.data(), buf.size());
    ASSERT_TRUE(decoded.input.has_value());
    EXPECT_EQ(decoded.status, InputParseStatus::Ok);
    EXPECT_EQ(decoded.input->playerId, p.playerId);
    EXPECT_EQ(decoded.input->flags, p.flags);
    EXPECT_EQ(decoded.input->sequenceId, p.header.sequenceId);
    EXPECT_EQ(decoded.input->tickId, p.header.tickId);
    EXPECT_FLOAT_EQ(decoded.input->x, p.x);
    EXPECT_FLOAT_EQ(decoded.input->y, p.y);
    EXPECT_FLOAT_EQ(decoded.input->angle, p.angle);
}

TEST(InputParser, RejectUnknownFlags)
{
    InputPacket p{};
    p.flags      = 0xFFFF;
    auto buf     = p.encode();
    auto decoded = parseInputPacket(buf.data(), buf.size());
    EXPECT_EQ(decoded.status, InputParseStatus::InvalidFlags);
    EXPECT_FALSE(decoded.input.has_value());
}

TEST(InputParser, RejectWrongSize)
{
    std::array<std::uint8_t, InputPacket::kSize - 1> buf{};
    auto decoded = parseInputPacket(buf.data(), buf.size());
    EXPECT_EQ(decoded.status, InputParseStatus::DecodeFailed);
    EXPECT_FALSE(decoded.input.has_value());
}

TEST(InputParser, RejectWrongMessageType)
{
    InputPacket p{};
    auto buf = p.encode();
    buf[6]   = static_cast<std::uint8_t>(MessageType::Snapshot);
    auto decoded = parseInputPacket(buf.data(), buf.size());
    EXPECT_EQ(decoded.status, InputParseStatus::DecodeFailed);
    EXPECT_FALSE(decoded.input.has_value());
}

TEST(InputParser, RejectWrongPacketType)
{
    InputPacket p{};
    auto buf = p.encode();
    buf[5]   = static_cast<std::uint8_t>(PacketType::ServerToClient);
    auto decoded = parseInputPacket(buf.data(), buf.size());
    EXPECT_EQ(decoded.status, InputParseStatus::DecodeFailed);
    EXPECT_FALSE(decoded.input.has_value());
}

TEST(InputParser, RejectCrcMismatch)
{
    InputPacket p{};
    auto buf = p.encode();
    buf.back() ^= 0xFF;
    auto decoded = parseInputPacket(buf.data(), buf.size());
    EXPECT_EQ(decoded.status, InputParseStatus::DecodeFailed);
    EXPECT_FALSE(decoded.input.has_value());
}
