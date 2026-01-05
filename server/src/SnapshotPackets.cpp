#include "components/InvincibilityComponent.hpp"
#include "components/LivesComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "server/EntityStateCache.hpp"
#include "server/EntityTypeResolver.hpp"
#include "server/Packets.hpp"

#include <algorithm>
#include <bit>
#include <cmath>

namespace
{
    constexpr float kPositionThreshold = 0.01F;
    constexpr float kVelocityThreshold = 0.01F;

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

    void writeI32(std::vector<std::uint8_t>& out, std::int32_t v)
    {
        writeU32(out, static_cast<std::uint32_t>(v));
    }

    void writeFloat(std::vector<std::uint8_t>& out, float f)
    {
        writeU32(out, std::bit_cast<std::uint32_t>(f));
    }

    CachedEntityState captureState(const Registry& registry, EntityId id)
    {
        CachedEntityState s{};
        const auto& t = registry.get<TransformComponent>(id);
        s.posX        = t.x;
        s.posY        = t.y;
        s.entityType  = resolveEntityType(registry, id);

        if (registry.has<VelocityComponent>(id)) {
            const auto& v = registry.get<VelocityComponent>(id);
            s.velX        = v.vx;
            s.velY        = v.vy;
        }
        if (registry.has<HealthComponent>(id))
            s.health = registry.get<HealthComponent>(id).current;
        if (registry.has<LivesComponent>(id))
            s.lives = registry.get<LivesComponent>(id).current;
        if (registry.has<ScoreComponent>(id))
            s.score = registry.get<ScoreComponent>(id).value;
        if (registry.has<InvincibilityComponent>(id))
            s.status |= (1 << 1);

        s.initialized = true;
        return s;
    }

    uint16_t calculateMask(const CachedEntityState& cur, const CachedEntityState* prev, const Registry& registry,
                           EntityId id, bool forceFull)
    {
        bool isDelta       = !forceFull && prev && prev->initialized;
        std::uint16_t mask = 0;

        if (!isDelta || cur.entityType != prev->entityType)
            mask |= 1 << 0;
        if (!isDelta || std::abs(cur.posX - prev->posX) > kPositionThreshold)
            mask |= 1 << 1;
        if (!isDelta || std::abs(cur.posY - prev->posY) > kPositionThreshold)
            mask |= 1 << 2;

        if (registry.has<VelocityComponent>(id)) {
            if (!isDelta || std::abs(cur.velX - prev->velX) > kVelocityThreshold)
                mask |= 1 << 3;
            if (!isDelta || std::abs(cur.velY - prev->velY) > kVelocityThreshold)
                mask |= 1 << 4;
        }
        if (registry.has<HealthComponent>(id) && (!isDelta || cur.health != prev->health))
            mask |= 1 << 5;
        if (registry.has<InvincibilityComponent>(id) && (!isDelta || cur.status != prev->status))
            mask |= 1 << 6;
        if (registry.has<LivesComponent>(id) && (!isDelta || cur.lives != prev->lives))
            mask |= 1 << 9;
        if (registry.has<ScoreComponent>(id) && (!isDelta || cur.score != prev->score))
            mask |= 1 << 10;

        return mask;
    }

    void writeDeltaData(std::vector<std::uint8_t>& block, uint16_t mask, const CachedEntityState& s)
    {
        if (mask & (1 << 0))
            block.push_back(s.entityType);
        if (mask & (1 << 1))
            writeFloat(block, s.posX);
        if (mask & (1 << 2))
            writeFloat(block, s.posY);
        if (mask & (1 << 3))
            writeFloat(block, s.velX);
        if (mask & (1 << 4))
            writeFloat(block, s.velY);
        if (mask & (1 << 5))
            writeU16(block, static_cast<std::uint16_t>(s.health));
        if (mask & (1 << 6))
            block.push_back(s.status);
        if (mask & (1 << 9))
            block.push_back(static_cast<std::uint8_t>(s.lives));
        if (mask & (1 << 10))
            writeI32(block, s.score);
    }

    std::vector<std::uint8_t> buildDeltaEntityBlock(const Registry& registry, EntityId id, EntityStateCache& cache,
                                                     bool forceFull)
    {
        auto cur  = captureState(registry, id);
        auto* old = cache.get(id);
        auto mask = calculateMask(cur, old, registry, id, forceFull);

        cache.update(id, cur);

        if (mask == 0)
            return {};

        std::vector<std::uint8_t> block;
        writeU32(block, id);
        writeU16(block, mask);
        writeDeltaData(block, mask, cur);

        return block;
    }

    std::vector<std::uint8_t> buildEntityBlock(const Registry& registry, EntityId id)
    {
        auto s    = captureState(registry, id);
        auto mask = calculateMask(s, nullptr, registry, id, true);

        std::vector<std::uint8_t> block;
        writeU32(block, id);
        writeU16(block, mask);
        writeDeltaData(block, mask, s);
        return block;
    }

    std::vector<SnapshotChunkBlock> buildBlocks(const Registry& registry, View<TransformComponent>& view)
    {
        std::vector<SnapshotChunkBlock> blocks;
        blocks.reserve(registry.entityCount());
        for (EntityId id : view)
            blocks.push_back(SnapshotChunkBlock{buildEntityBlock(registry, id)});
        return blocks;
    }

    std::vector<SnapshotChunkPacket> buildChunksFromBlocks(const std::vector<SnapshotChunkBlock>& blocks,
                                                           std::size_t maxPayloadBytes)
    {
        std::vector<SnapshotChunkPacket> chunks;
        chunks.emplace_back();
        for (const auto& blk : blocks) {
            if (!chunks.empty() && chunks.back().data.size() + blk.data.size() + 6 > maxPayloadBytes)
                chunks.emplace_back();
            chunks.back().data.insert(chunks.back().data.end(), blk.data.begin(), blk.data.end());
            chunks.back().entityCount++;
        }
        if (!chunks.empty() && chunks.back().entityCount == 0)
            chunks.pop_back();
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

        auto hdrBytes = hdr.encode();
        std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
        out.insert(out.end(), payload.begin(), payload.end());
        writeU32(out, PacketHeader::crc32(out.data(), out.size()));
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

    auto hdrBytes = hdr.encode();
    std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
    out.insert(out.end(), payload.begin(), payload.end());
    writeU32(out, PacketHeader::crc32(out.data(), out.size()));
    return out;
}

std::vector<std::vector<std::uint8_t>> buildSnapshotChunks(Registry& registry, uint32_t tick,
                                                           std::size_t maxPayloadBytes)
{
    auto view         = registry.view<TransformComponent>();
    const auto blocks = buildBlocks(registry, view);
    const auto chunks = buildChunksFromBlocks(blocks, maxPayloadBytes);

    std::vector<std::vector<std::uint8_t>> packets;
    const std::uint16_t totalChunks = static_cast<std::uint16_t>(chunks.size());
    packets.reserve(totalChunks);
    for (std::uint16_t idx = 0; idx < totalChunks; ++idx)
        packets.push_back(buildChunkPacket(chunks[idx], totalChunks, idx, tick));
    return packets;
}

std::vector<std::uint8_t> buildDeltaSnapshotPacket(Registry& registry, uint32_t tick,
                                                    EntityStateCache& cache, bool forceFullState)
{
    auto view = registry.view<TransformComponent>();
    std::vector<std::uint8_t> payload;
    payload.reserve(registry.entityCount() * 8);

    std::vector<std::vector<std::uint8_t>> entityBlocks;
    entityBlocks.reserve(registry.entityCount());

    for (EntityId id : view) {
        auto block = buildDeltaEntityBlock(registry, id, cache, forceFullState);
        if (!block.empty())
            entityBlocks.push_back(std::move(block));
    }

    writeU16(payload, static_cast<std::uint16_t>(entityBlocks.size()));
    for (const auto& block : entityBlocks)
        payload.insert(payload.end(), block.begin(), block.end());

    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::Snapshot);
    hdr.sequenceId  = static_cast<std::uint16_t>(tick & 0xFFFF);
    hdr.tickId      = tick;
    hdr.payloadSize = static_cast<std::uint16_t>(payload.size());

    auto hdrBytes = hdr.encode();
    std::vector<std::uint8_t> out(hdrBytes.begin(), hdrBytes.end());
    out.insert(out.end(), payload.begin(), payload.end());
    writeU32(out, PacketHeader::crc32(out.data(), out.size()));
    return out;
}


