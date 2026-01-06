#include "network/PacketHeader.hpp"
#include "network/Packing.hpp"
#include "network/SnapshotParser.hpp"

#include <bit>
#include <gtest/gtest.h>

namespace
{
    std::vector<std::uint8_t> buildSnapshot(std::uint16_t entityCount,
                                            const std::vector<std::vector<std::uint8_t>>& entityPayloads)
    {
        PacketHeader h{};
        h.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        h.messageType = static_cast<std::uint8_t>(MessageType::Snapshot);

        std::vector<std::uint8_t> buf;
        auto hdr = h.encode();
        buf.insert(buf.end(), hdr.begin(), hdr.end());
        buf.push_back(static_cast<std::uint8_t>((entityCount >> 8) & 0xFF));
        buf.push_back(static_cast<std::uint8_t>(entityCount & 0xFF));
        for (auto& payload : entityPayloads) {
            buf.insert(buf.end(), payload.begin(), payload.end());
        }
        std::size_t payloadSize = buf.size() - PacketHeader::kSize;
        buf[13]                 = static_cast<std::uint8_t>((payloadSize >> 8) & 0xFF);
        buf[14]                 = static_cast<std::uint8_t>(payloadSize & 0xFF);

        std::uint32_t crc = PacketHeader::crc32(buf.data(), buf.size());
        buf.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
        buf.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
        buf.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
        buf.push_back(static_cast<std::uint8_t>(crc & 0xFF));
        return buf;
    }

    std::vector<std::uint8_t> entityPayload(std::uint32_t id, std::uint16_t mask, const std::vector<std::uint8_t>& data)
    {
        std::vector<std::uint8_t> out;
        out.push_back(static_cast<std::uint8_t>((id >> 24) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((id >> 16) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((id >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(id & 0xFF));
        out.push_back(static_cast<std::uint8_t>((mask >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(mask & 0xFF));
        out.insert(out.end(), data.begin(), data.end());
        return out;
    }

    void writeQ16(std::vector<std::uint8_t>& out, float v)
    {
        std::int16_t q = Packing::quantizeTo16(v, 10.0F);
        out.push_back(static_cast<std::uint8_t>((q >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(q & 0xFF));
    }

    void writeFloat(std::vector<std::uint8_t>& out, float v)
    {
        auto bits = std::bit_cast<std::uint32_t>(v);
        out.push_back(static_cast<std::uint8_t>((bits >> 24) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((bits >> 16) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((bits >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(bits & 0xFF));
    }

    void writeU16(std::vector<std::uint8_t>& out, std::uint16_t v)
    {
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }
} // namespace

TEST(SnapshotParser, ParsesSingleEntityWithFields)
{
    std::vector<std::uint8_t> data;
    data.push_back(7);
    writeQ16(data, 1.5F);
    writeQ16(data, -2.5F);
    writeQ16(data, 0.5F);
    writeQ16(data, -0.25F);
    writeU16(data, 50);
    data.push_back(Packing::pack44(3, 5)); // status=3, lives=5
    writeFloat(data, 0.1F);
    data.push_back(1);

    auto payload = entityPayload(42, 0x1FF, data); // 0x1FF = bits 0-8 (covering all fields, bit 9 removed)
    auto pkt     = buildSnapshot(1, {payload});

    auto parsed = SnapshotParser::parse(pkt);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_EQ(parsed->entities.size(), 1u);
    const auto& e = parsed->entities[0];
    EXPECT_EQ(e.entityId, 42u);
    EXPECT_TRUE(e.entityType.has_value());
    EXPECT_EQ(*e.entityType, 7u);
    EXPECT_NEAR(*e.posX, 1.5F, 0.11F);
    EXPECT_NEAR(*e.posY, -2.5F, 0.11F);
    EXPECT_NEAR(*e.velX, 0.5F, 0.11F);
    EXPECT_NEAR(*e.velY, -0.25F, 0.11F);
    EXPECT_TRUE(e.health.has_value());
    EXPECT_EQ(*e.health, 50);
    EXPECT_TRUE(e.statusEffects.has_value());
    EXPECT_EQ(*e.statusEffects, 3u);
    EXPECT_TRUE(e.lives.has_value());
    EXPECT_EQ(*e.lives, 5);
    EXPECT_TRUE(e.orientation.has_value());
    EXPECT_NEAR(*e.orientation, 0.1F, 1e-5F);
    EXPECT_TRUE(e.dead.has_value());
    EXPECT_TRUE(*e.dead);
}

TEST(SnapshotParser, RejectsWrongPacketType)
{
    auto pkt    = buildSnapshot(0, {});
    pkt[5]      = static_cast<std::uint8_t>(PacketType::ClientToServer);
    auto parsed = SnapshotParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}

TEST(SnapshotParser, RejectsWrongMessageType)
{
    auto pkt    = buildSnapshot(0, {});
    pkt[6]      = static_cast<std::uint8_t>(MessageType::Input);
    auto parsed = SnapshotParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}

TEST(SnapshotParser, RejectsCrcMismatch)
{
    auto pkt = buildSnapshot(0, {});
    pkt.back() ^= 0xFF;
    auto parsed = SnapshotParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}

TEST(SnapshotParser, ParsesZeroEntities)
{
    auto pkt    = buildSnapshot(0, {});
    auto parsed = SnapshotParser::parse(pkt);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_TRUE(parsed->entities.empty());
}

TEST(SnapshotParser, RejectsTruncatedEntityHeader)
{
    auto payload = entityPayload(1, 0xFFFF, {});
    payload.resize(5);
    auto pkt    = buildSnapshot(1, {payload});
    auto parsed = SnapshotParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}

TEST(SnapshotParser, RejectsMissingFieldData)
{
    std::vector<std::uint8_t> data;
    auto payload = entityPayload(1, 0x001, data);
    auto pkt     = buildSnapshot(1, {payload});
    auto parsed  = SnapshotParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}

TEST(SnapshotParser, RejectsPayloadTooShortForCount)
{
    auto pkt    = buildSnapshot(2, {});
    auto parsed = SnapshotParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}

TEST(SnapshotParser, ParsesMultipleEntities)
{
    std::vector<std::uint8_t> e1data;
    e1data.push_back(2);
    writeQ16(e1data, 10.0F);
    auto e1 = entityPayload(10, 0x003, e1data);

    std::vector<std::uint8_t> e2data;
    writeQ16(e2data, -1.0F);
    writeQ16(e2data, 2.0F);
    auto e2 = entityPayload(20, 0x00C, e2data);

    auto pkt    = buildSnapshot(2, {e1, e2});
    auto parsed = SnapshotParser::parse(pkt);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_EQ(parsed->entities.size(), 2u);
    EXPECT_EQ(parsed->entities[0].entityId, 10u);
    EXPECT_TRUE(parsed->entities[0].entityType.has_value());
    EXPECT_NEAR(*parsed->entities[0].posX, 10.0F, 0.11F);
    EXPECT_FALSE(parsed->entities[0].posY.has_value());
    EXPECT_EQ(parsed->entities[1].entityId, 20u);
    EXPECT_FALSE(parsed->entities[1].entityType.has_value());
    EXPECT_TRUE(parsed->entities[1].posY.has_value());
    EXPECT_NEAR(*parsed->entities[1].posY, -1.0F, 0.11F);
    EXPECT_TRUE(parsed->entities[1].velX.has_value());
    EXPECT_NEAR(*parsed->entities[1].velX, 2.0F, 0.11F);
}

TEST(SnapshotParser, RejectsIfBufferSmallerThanHeaderPlusCrc)
{
    std::vector<std::uint8_t> pkt(PacketHeader::kSize + PacketHeader::kCrcSize - 1, 0);
    auto parsed = SnapshotParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}
