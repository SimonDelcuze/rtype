#include "concurrency/ThreadSafeQueue.hpp"
#include "network/LevelInitData.hpp"
#include "network/NetworkMessageHandler.hpp"
#include "network/PacketHeader.hpp"
#include "network/SnapshotParser.hpp"

#include <bit>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    void writeU16(std::vector<std::uint8_t>& out, std::uint16_t v)
    {
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }

    void writeU32(std::vector<std::uint8_t>& out, std::uint32_t v)
    {
        out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }

    void writeFloat(std::vector<std::uint8_t>& out, float v)
    {
        writeU32(out, std::bit_cast<std::uint32_t>(v));
    }

    std::vector<std::uint8_t> makeSnapshotPacket()
    {
        PacketHeader h{};
        h.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        h.messageType = static_cast<std::uint8_t>(MessageType::Snapshot);

        std::vector<std::uint8_t> buf;
        auto hdr = h.encode();
        buf.insert(buf.end(), hdr.begin(), hdr.end());

        writeU16(buf, 1);
        writeU32(buf, 123);
        writeU16(buf, 0x00C);
        writeFloat(buf, -5.0F);
        writeFloat(buf, 10.0F);

        std::size_t payloadSize = buf.size() - PacketHeader::kSize;
        buf[13]                 = static_cast<std::uint8_t>((payloadSize >> 8) & 0xFF);
        buf[14]                 = static_cast<std::uint8_t>(payloadSize & 0xFF);

        std::uint32_t crc = PacketHeader::crc32(buf.data(), buf.size());
        writeU32(buf, crc);
        return buf;
    }

    std::vector<std::uint8_t> makeNonSnapshotPacket()
    {
        PacketHeader h{};
        h.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        h.messageType = static_cast<std::uint8_t>(MessageType::Input);
        std::vector<std::uint8_t> buf;
        auto hdr = h.encode();
        buf.insert(buf.end(), hdr.begin(), hdr.end());
        std::size_t payloadSize = 0;
        buf[13]                 = 0;
        buf[14]                 = 0;
        std::uint32_t crc       = PacketHeader::crc32(buf.data(), buf.size());
        writeU32(buf, crc);
        return buf;
    }
} // namespace

TEST(NetworkMessageHandler, DispatchesSnapshotToParsedQueue)
{
    ThreadSafeQueue<std::vector<std::uint8_t>> raw;
    ThreadSafeQueue<SnapshotParseResult> parsed;
    ThreadSafeQueue<LevelInitData> levelInit;
    NetworkMessageHandler handler(raw, parsed, levelInit);

    raw.push(makeSnapshotPacket());
    handler.poll();

    SnapshotParseResult out;
    EXPECT_TRUE(parsed.tryPop(out));
    ASSERT_EQ(out.entities.size(), 1u);
    EXPECT_EQ(out.entities[0].entityId, 123u);
    EXPECT_TRUE(out.entities[0].posY.has_value());
    EXPECT_NEAR(*out.entities[0].posY, -5.0F, 1e-5F);
    EXPECT_TRUE(out.entities[0].velX.has_value());
    EXPECT_NEAR(*out.entities[0].velX, 10.0F, 1e-5F);
}

TEST(NetworkMessageHandler, IgnoresNonSnapshot)
{
    ThreadSafeQueue<std::vector<std::uint8_t>> raw;
    ThreadSafeQueue<SnapshotParseResult> parsed;
    ThreadSafeQueue<LevelInitData> levelInit;
    NetworkMessageHandler handler(raw, parsed, levelInit);

    raw.push(makeNonSnapshotPacket());
    handler.poll();

    SnapshotParseResult out;
    EXPECT_FALSE(parsed.tryPop(out));
}

TEST(NetworkMessageHandler, IgnoresInvalidHeader)
{
    ThreadSafeQueue<std::vector<std::uint8_t>> raw;
    ThreadSafeQueue<SnapshotParseResult> parsed;
    ThreadSafeQueue<LevelInitData> levelInit;
    NetworkMessageHandler handler(raw, parsed, levelInit);

    std::vector<std::uint8_t> pkt(10, 0);
    raw.push(pkt);
    handler.poll();

    SnapshotParseResult out;
    EXPECT_FALSE(parsed.tryPop(out));
}

TEST(NetworkMessageHandler, IgnoresCrcMismatch)
{
    ThreadSafeQueue<std::vector<std::uint8_t>> raw;
    ThreadSafeQueue<SnapshotParseResult> parsed;
    ThreadSafeQueue<LevelInitData> levelInit;
    NetworkMessageHandler handler(raw, parsed, levelInit);

    auto pkt = makeSnapshotPacket();
    pkt.back() ^= 0xFF;
    raw.push(pkt);
    handler.poll();

    SnapshotParseResult out;
    EXPECT_FALSE(parsed.tryPop(out));
}

TEST(NetworkMessageHandler, NoCrashOnEmptyQueue)
{
    ThreadSafeQueue<std::vector<std::uint8_t>> raw;
    ThreadSafeQueue<SnapshotParseResult> parsed;
    ThreadSafeQueue<LevelInitData> levelInit;
    NetworkMessageHandler handler(raw, parsed, levelInit);
    handler.poll();
    SnapshotParseResult out;
    EXPECT_FALSE(parsed.tryPop(out));
}
