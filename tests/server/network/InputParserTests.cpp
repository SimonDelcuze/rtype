#include "network/InputParser.hpp"

#include <gtest/gtest.h>

TEST(InputParser, ParseValidPacket)
{
    InputPacket p{};
    p.header.sequenceId = 0x42;
    p.header.tickId = 0x01020304;
    p.playerId = 7;
    p.flags = static_cast<std::uint16_t>(InputFlag::MoveUp | InputFlag::Fire);
    p.x = 10.0F;
    p.y = -5.5F;
    p.angle = 1.25F;
    auto buf = p.encode();

    auto decoded = parseInputPacket(buf.data(), buf.size());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->playerId, p.playerId);
    EXPECT_EQ(decoded->flags, p.flags);
    EXPECT_EQ(decoded->sequenceId, p.header.sequenceId);
    EXPECT_EQ(decoded->tickId, p.header.tickId);
    EXPECT_FLOAT_EQ(decoded->x, p.x);
    EXPECT_FLOAT_EQ(decoded->y, p.y);
    EXPECT_FLOAT_EQ(decoded->angle, p.angle);
}

TEST(InputParser, RejectUnknownFlags)
{
    InputPacket p{};
    p.flags = 0xFFFF;
    auto buf = p.encode();
    auto decoded = parseInputPacket(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}

TEST(InputParser, RejectWrongSize)
{
    std::array<std::uint8_t, InputPacket::kSize - 1> buf{};
    auto decoded = parseInputPacket(buf.data(), buf.size());
    EXPECT_FALSE(decoded.has_value());
}
