#include "network/DeltaStatePacket.hpp"

#include <gtest/gtest.h>

TEST(DeltaStatePacket, EncodeDecodeRoundTrip)
{
    DeltaStatePacket pkt{};
    pkt.header.sequenceId = 0x1234;
    pkt.header.tickId     = 0x0A0B0C0D;
    pkt.entries.push_back(DeltaEntry{1, 1.5F, -2.0F, 3.0F, -4.0F, 10});
    pkt.entries.push_back(DeltaEntry{2, 0.0F, 0.0F, 0.0F, 0.0F, -5});

    auto buf = pkt.encode();
    auto dec = DeltaStatePacket::decode(buf.data(), buf.size());
    ASSERT_TRUE(dec.has_value());
    EXPECT_EQ(dec->header.messageType, static_cast<std::uint8_t>(MessageType::Snapshot));
    EXPECT_EQ(dec->header.sequenceId, pkt.header.sequenceId);
    EXPECT_EQ(dec->header.tickId, pkt.header.tickId);
    ASSERT_EQ(dec->entries.size(), pkt.entries.size());
    for (std::size_t i = 0; i < pkt.entries.size(); ++i) {
        EXPECT_EQ(dec->entries[i].entityId, pkt.entries[i].entityId);
        EXPECT_FLOAT_EQ(dec->entries[i].posX, pkt.entries[i].posX);
        EXPECT_FLOAT_EQ(dec->entries[i].posY, pkt.entries[i].posY);
        EXPECT_FLOAT_EQ(dec->entries[i].velX, pkt.entries[i].velX);
        EXPECT_FLOAT_EQ(dec->entries[i].velY, pkt.entries[i].velY);
        EXPECT_EQ(dec->entries[i].hp, pkt.entries[i].hp);
    }
}

TEST(DeltaStatePacket, RejectsWrongType)
{
    DeltaStatePacket pkt{};
    auto buf = pkt.encode();
    buf[0]   = static_cast<std::uint8_t>(MessageType::Input);
    auto dec = DeltaStatePacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(dec.has_value());
}

TEST(DeltaStatePacket, RejectsWrongSize)
{
    std::vector<std::uint8_t> buf(PacketHeader::kSize + 1, 0);
    auto dec = DeltaStatePacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(dec.has_value());
}

TEST(DeltaStatePacket, RejectsMismatchedLength)
{
    DeltaStatePacket pkt{};
    pkt.entries.push_back(DeltaEntry{1, 0.0F, 0.0F, 0.0F, 0.0F, 0});
    auto buf = pkt.encode();
    buf.push_back(0);
    auto dec = DeltaStatePacket::decode(buf.data(), buf.size());
    EXPECT_FALSE(dec.has_value());
}
