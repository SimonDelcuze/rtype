#include "server/Packets.hpp"

#include <bit>

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

    void writeFloat(std::vector<std::uint8_t>& out, float f)
    {
        writeU32(out, std::bit_cast<std::uint32_t>(f));
    }

    std::uint16_t typeForEntity(const Registry& registry, EntityId id)
    {
        if (registry.has<TagComponent>(id) && registry.get<TagComponent>(id).hasTag(EntityTag::Player))
            return 1;
        if (registry.has<TagComponent>(id) && registry.get<TagComponent>(id).hasTag(EntityTag::Projectile))
            return 3;
        return 2;
    }
} // namespace

std::vector<std::uint8_t> buildSnapshotPacket(Registry& registry, uint32_t tick)
{
    std::vector<std::uint8_t> payload;
    auto view           = registry.view<TransformComponent>();
    std::uint16_t count = 0;
    for (auto id : view) {
        (void) id;
        ++count;
    }
    writeU16(payload, count);
    for (EntityId id : view) {
        writeU32(payload, id);
        std::uint16_t mask = 0;
        mask |= 1 << 0;
        mask |= 1 << 1;
        mask |= 1 << 2;
        if (registry.has<VelocityComponent>(id)) {
            mask |= 1 << 3;
            mask |= 1 << 4;
        }
        if (registry.has<HealthComponent>(id))
            mask |= 1 << 5;
        writeU16(payload, mask);
        payload.push_back(static_cast<std::uint8_t>(typeForEntity(registry, id)));

        const auto& t = registry.get<TransformComponent>(id);
        writeFloat(payload, t.x);
        writeFloat(payload, t.y);

        if (mask & (1 << 3)) {
            const auto& v = registry.get<VelocityComponent>(id);
            writeFloat(payload, v.vx);
            writeFloat(payload, v.vy);
        }
        if (mask & (1 << 5)) {
            const auto& h = registry.get<HealthComponent>(id);
            writeU16(payload, static_cast<std::uint16_t>(h.current));
        }
    }

    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::Snapshot);
    hdr.sequenceId  = static_cast<std::uint16_t>(tick & 0xFFFF);
    hdr.tickId      = tick;
    hdr.payloadSize = static_cast<std::uint16_t>(payload.size());
    auto hdrBytes   = hdr.encode();
    std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
    out.insert(out.end(), payload.begin(), payload.end());
    auto crc = PacketHeader::crc32(out.data(), out.size());
    writeU32(out, crc);
    return out;
}

std::vector<std::vector<std::uint8_t>> buildSnapshotChunks(Registry& registry, uint32_t tick,
                                                           std::size_t maxPayloadBytes)
{
    struct Block
    {
        std::vector<std::uint8_t> data;
    };

    std::vector<Block> blocks;
    auto view = registry.view<TransformComponent>();
    for (EntityId id : view) {
        Block b;
        writeU32(b.data, id);
        std::uint16_t mask = 0;
        mask |= 1 << 0;
        mask |= 1 << 1;
        mask |= 1 << 2;
        if (registry.has<VelocityComponent>(id)) {
            mask |= 1 << 3;
            mask |= 1 << 4;
        }
        if (registry.has<HealthComponent>(id))
            mask |= 1 << 5;
        writeU16(b.data, mask);
        b.data.push_back(static_cast<std::uint8_t>(typeForEntity(registry, id)));

        const auto& t = registry.get<TransformComponent>(id);
        writeFloat(b.data, t.x);
        writeFloat(b.data, t.y);

        if (mask & (1 << 3)) {
            const auto& v = registry.get<VelocityComponent>(id);
            writeFloat(b.data, v.vx);
            writeFloat(b.data, v.vy);
        }
        if (mask & (1 << 5)) {
            const auto& h = registry.get<HealthComponent>(id);
            writeU16(b.data, static_cast<std::uint16_t>(h.current));
        }
        blocks.push_back(std::move(b));
    }

    struct Chunk
    {
        std::vector<std::uint8_t> data;
        std::uint16_t entityCount = 0;
    };

    std::vector<Chunk> chunks;
    chunks.emplace_back();
    for (const auto& blk : blocks) {
        if (!chunks.empty() && chunks.back().data.size() + blk.data.size() + 6 > maxPayloadBytes) {
            chunks.emplace_back();
        }
        chunks.back().data.insert(chunks.back().data.end(), blk.data.begin(), blk.data.end());
        chunks.back().entityCount++;
    }
    if (!chunks.empty() && chunks.back().entityCount == 0) {
        chunks.pop_back();
    }

    std::vector<std::vector<std::uint8_t>> packets;
    const std::uint16_t totalChunks = static_cast<std::uint16_t>(chunks.size());
    for (std::uint16_t idx = 0; idx < totalChunks; ++idx) {
        const auto& ch = chunks[idx];
        std::vector<std::uint8_t> payload;
        writeU16(payload, totalChunks);
        writeU16(payload, idx);
        writeU16(payload, ch.entityCount);
        payload.insert(payload.end(), ch.data.begin(), ch.data.end());

        PacketHeader hdr{};
        hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        hdr.messageType = static_cast<std::uint8_t>(MessageType::SnapshotChunk);
        hdr.sequenceId  = static_cast<std::uint16_t>((tick + idx) & 0xFFFF);
        hdr.tickId      = tick;
        hdr.payloadSize = static_cast<std::uint16_t>(payload.size());
        auto hdrBytes   = hdr.encode();
        std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
        out.insert(out.end(), payload.begin(), payload.end());
        auto crc = PacketHeader::crc32(out.data(), out.size());
        writeU32(out, crc);
        packets.push_back(std::move(out));
    }
    return packets;
}
