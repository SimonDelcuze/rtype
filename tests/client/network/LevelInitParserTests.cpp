#include "network/LevelInitData.hpp"
#include "network/LevelInitParser.hpp"
#include "network/PacketHeader.hpp"

#include <gtest/gtest.h>

namespace
{
    void writeU8(std::vector<std::uint8_t>& buf, std::uint8_t v)
    {
        buf.push_back(v);
    }

    void writeU16(std::vector<std::uint8_t>& buf, std::uint16_t v)
    {
        buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        buf.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }

    void writeU32(std::vector<std::uint8_t>& buf, std::uint32_t v)
    {
        buf.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
        buf.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
        buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        buf.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }

    void writeString(std::vector<std::uint8_t>& buf, const std::string& s)
    {
        buf.push_back(static_cast<std::uint8_t>(s.size()));
        buf.insert(buf.end(), s.begin(), s.end());
    }

    std::vector<std::uint8_t> buildLevelInit(std::uint16_t levelId, std::uint32_t seed, const std::string& bgId,
                                             const std::string& musicId, const std::vector<ArchetypeEntry>& archetypes)
    {
        PacketHeader h{};
        h.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        h.messageType = static_cast<std::uint8_t>(MessageType::LevelInit);

        std::vector<std::uint8_t> buf;
        auto hdr = h.encode();
        buf.insert(buf.end(), hdr.begin(), hdr.end());

        writeU16(buf, levelId);
        writeU32(buf, seed);
        writeString(buf, bgId);
        writeString(buf, musicId);
        writeU8(buf, static_cast<std::uint8_t>(archetypes.size()));

        for (const auto& a : archetypes) {
            writeU16(buf, a.typeId);
            writeString(buf, a.spriteId);
            writeString(buf, a.animId);
            writeU8(buf, a.layer);
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
} // namespace

TEST(LevelInitParser, ParsesValidPacket)
{
    ArchetypeEntry a1{};
    a1.typeId   = 1;
    a1.spriteId = "player_ship";
    a1.animId   = "idle";
    a1.layer    = 5;

    ArchetypeEntry a2{};
    a2.typeId   = 2;
    a2.spriteId = "enemy_ship";
    a2.animId   = "";
    a2.layer    = 3;

    auto pkt = buildLevelInit(42, 12345, "space_bg", "level1_music", {a1, a2});

    auto parsed = LevelInitParser::parse(pkt);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->levelId, 42);
    EXPECT_EQ(parsed->seed, 12345u);
    EXPECT_EQ(parsed->backgroundId, "space_bg");
    EXPECT_EQ(parsed->musicId, "level1_music");
    ASSERT_EQ(parsed->archetypes.size(), 2u);

    EXPECT_EQ(parsed->archetypes[0].typeId, 1);
    EXPECT_EQ(parsed->archetypes[0].spriteId, "player_ship");
    EXPECT_EQ(parsed->archetypes[0].animId, "idle");
    EXPECT_EQ(parsed->archetypes[0].layer, 5);

    EXPECT_EQ(parsed->archetypes[1].typeId, 2);
    EXPECT_EQ(parsed->archetypes[1].spriteId, "enemy_ship");
    EXPECT_EQ(parsed->archetypes[1].animId, "");
    EXPECT_EQ(parsed->archetypes[1].layer, 3);
}

TEST(LevelInitParser, ParsesEmptyArchetypes)
{
    auto pkt    = buildLevelInit(1, 999, "bg", "music", {});
    auto parsed = LevelInitParser::parse(pkt);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->levelId, 1);
    EXPECT_TRUE(parsed->archetypes.empty());
}

TEST(LevelInitParser, ParsesEmptyStrings)
{
    auto pkt    = buildLevelInit(100, 0, "", "", {});
    auto parsed = LevelInitParser::parse(pkt);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->backgroundId, "");
    EXPECT_EQ(parsed->musicId, "");
}

TEST(LevelInitParser, RejectsWrongMessageType)
{
    auto pkt    = buildLevelInit(1, 1, "a", "b", {});
    pkt[6]      = static_cast<std::uint8_t>(MessageType::Snapshot);
    auto parsed = LevelInitParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}

TEST(LevelInitParser, RejectsWrongPacketType)
{
    auto pkt    = buildLevelInit(1, 1, "a", "b", {});
    pkt[5]      = static_cast<std::uint8_t>(PacketType::ClientToServer);
    auto parsed = LevelInitParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}

TEST(LevelInitParser, RejectsTooShortPacket)
{
    std::vector<std::uint8_t> pkt(10, 0);
    auto parsed = LevelInitParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}

TEST(LevelInitParser, RejectsTruncatedPayload)
{
    auto pkt = buildLevelInit(1, 1, "background", "music", {});
    pkt.resize(PacketHeader::kSize + 5);
    auto parsed = LevelInitParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}

TEST(LevelInitParser, RejectsMissingArchetypeData)
{
    ArchetypeEntry a{};
    a.typeId   = 1;
    a.spriteId = "sprite";
    a.animId   = "anim";
    a.layer    = 1;

    auto pkt = buildLevelInit(1, 1, "bg", "m", {a});
    pkt.resize(pkt.size() - 10);
    auto parsed = LevelInitParser::parse(pkt);
    EXPECT_FALSE(parsed.has_value());
}

TEST(LevelInitParser, ParsesMaxLevelId)
{
    auto pkt    = buildLevelInit(0xFFFF, 0xFFFFFFFF, "bg", "m", {});
    auto parsed = LevelInitParser::parse(pkt);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->levelId, 0xFFFF);
    EXPECT_EQ(parsed->seed, 0xFFFFFFFF);
}
