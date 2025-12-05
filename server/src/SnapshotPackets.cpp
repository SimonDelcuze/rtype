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
    auto view = registry.view<TransformComponent>();
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
