#include "server/Packets.hpp"

#include <algorithm>
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
        if (registry.has<TagComponent>(id) && registry.get<TagComponent>(id).hasTag(EntityTag::Projectile)) {
            int charge = 1;
            if (registry.has<MissileComponent>(id)) {
                charge = std::clamp(registry.get<MissileComponent>(id).chargeLevel, 1, 5);
            }
            switch (charge) {
                case 1:
                    return 3;
                case 2:
                    return 4;
                case 3:
                    return 5;
                case 4:
                    return 6;
                case 5:
                default:
                    return 8;
            }
        }
        return 2;
    }

    std::vector<std::uint8_t> buildEntityBlock(const Registry& registry, EntityId id)
    {
        std::vector<std::uint8_t> block;
        writeU32(block, id);
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
        writeU16(block, mask);
        block.push_back(static_cast<std::uint8_t>(typeForEntity(registry, id)));

        const auto& t = registry.get<TransformComponent>(id);
        writeFloat(block, t.x);
        writeFloat(block, t.y);

        if (mask & (1 << 3)) {
            const auto& v = registry.get<VelocityComponent>(id);
            writeFloat(block, v.vx);
            writeFloat(block, v.vy);
        }
        if (mask & (1 << 5)) {
            const auto& h = registry.get<HealthComponent>(id);
            writeU16(block, static_cast<std::uint16_t>(h.current));
        }
        return block;
    }

    std::vector<SnapshotChunkBlock> buildBlocks(const Registry& registry, View<TransformComponent>& view)
    {
        std::vector<SnapshotChunkBlock> blocks;
        blocks.reserve(registry.entityCount());
        for (EntityId id : view) {
            blocks.push_back(SnapshotChunkBlock{buildEntityBlock(registry, id)});
        }
        return blocks;
    }

    std::vector<SnapshotChunkPacket> buildChunksFromBlocks(const std::vector<SnapshotChunkBlock>& blocks,
                                                           std::size_t maxPayloadBytes)
    {
        std::vector<SnapshotChunkPacket> chunks;
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
        return chunks;
    }

    std::vector<std::uint8_t> buildChunkPacket(const SnapshotChunkPacket& ch, std::uint16_t totalChunks,
                                               std::uint16_t idx, uint32_t tick)
    {
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
        return out;
    }
} // namespace

std::vector<std::uint8_t> buildSnapshotPacket(Registry& registry, uint32_t tick)
{
    auto view = registry.view<TransformComponent>();
    std::vector<std::uint8_t> payload;
    payload.reserve(registry.entityCount() * 16);

    std::uint16_t count = 0;
    for ([[maybe_unused]] auto id : view)
        ++count;
    writeU16(payload, count);

    for (EntityId id : view) {
        auto block = buildEntityBlock(registry, id);
        payload.insert(payload.end(), block.begin(), block.end());
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
    auto view = registry.view<TransformComponent>();

    const auto blocks = buildBlocks(registry, view);
    const auto chunks = buildChunksFromBlocks(blocks, maxPayloadBytes);

    std::vector<std::vector<std::uint8_t>> packets;
    const std::uint16_t totalChunks = static_cast<std::uint16_t>(chunks.size());
    packets.reserve(totalChunks);
    for (std::uint16_t idx = 0; idx < totalChunks; ++idx) {
        packets.push_back(buildChunkPacket(chunks[idx], totalChunks, idx, tick));
    }
    return packets;
}
