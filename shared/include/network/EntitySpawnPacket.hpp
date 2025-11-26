#pragma once

#include "network/PacketHeader.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <optional>

struct EntitySpawnPacket
{
    PacketHeader header{};
    std::uint32_t entityId  = 0;
    std::uint8_t entityType = 0;
    float posX              = 0.0F;
    float posY              = 0.0F;

    static constexpr std::size_t kPayloadSize = 4 + 1 + 4 + 4;
    static constexpr std::size_t kSize        = PacketHeader::kSize + kPayloadSize + PacketHeader::kCrcSize;

    [[nodiscard]] std::array<std::uint8_t, kSize> encode() const noexcept
    {
        PacketHeader h = header;
        h.version      = PacketHeader::kProtocolVersion;
        h.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
        h.messageType  = static_cast<std::uint8_t>(MessageType::EntitySpawn);
        h.payloadSize  = static_cast<std::uint16_t>(kPayloadSize);
        auto hdr       = h.encode();
        std::array<std::uint8_t, kSize> out{};
        for (std::size_t i = 0; i < hdr.size(); ++i)
            out[i] = hdr[i];
        std::size_t o = PacketHeader::kSize;
        out[o]        = static_cast<std::uint8_t>((entityId >> 24) & 0xFF);
        out[o + 1]    = static_cast<std::uint8_t>((entityId >> 16) & 0xFF);
        out[o + 2]    = static_cast<std::uint8_t>((entityId >> 8) & 0xFF);
        out[o + 3]    = static_cast<std::uint8_t>(entityId & 0xFF);
        o += 4;
        out[o] = entityType;
        o += 1;
        auto wx    = std::bit_cast<std::uint32_t>(posX);
        auto wy    = std::bit_cast<std::uint32_t>(posY);
        out[o]     = static_cast<std::uint8_t>((wx >> 24) & 0xFF);
        out[o + 1] = static_cast<std::uint8_t>((wx >> 16) & 0xFF);
        out[o + 2] = static_cast<std::uint8_t>((wx >> 8) & 0xFF);
        out[o + 3] = static_cast<std::uint8_t>(wx & 0xFF);
        o += 4;
        out[o]     = static_cast<std::uint8_t>((wy >> 24) & 0xFF);
        out[o + 1] = static_cast<std::uint8_t>((wy >> 16) & 0xFF);
        out[o + 2] = static_cast<std::uint8_t>((wy >> 8) & 0xFF);
        out[o + 3] = static_cast<std::uint8_t>(wy & 0xFF);
        o += 4;
        auto crc   = PacketHeader::crc32(out.data(), PacketHeader::kSize + kPayloadSize);
        out[o]     = static_cast<std::uint8_t>((crc >> 24) & 0xFF);
        out[o + 1] = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
        out[o + 2] = static_cast<std::uint8_t>((crc >> 8) & 0xFF);
        out[o + 3] = static_cast<std::uint8_t>(crc & 0xFF);
        return out;
    }

    [[nodiscard]] static std::optional<EntitySpawnPacket> decode(const std::uint8_t* data, std::size_t len) noexcept
    {
        if (data == nullptr || len < kSize)
            return std::nullopt;
        auto hdr = PacketHeader::decode(data, len);
        if (!hdr)
            return std::nullopt;
        if (hdr->messageType != static_cast<std::uint8_t>(MessageType::EntitySpawn))
            return std::nullopt;
        if (hdr->packetType != static_cast<std::uint8_t>(PacketType::ServerToClient))
            return std::nullopt;
        if (hdr->payloadSize != kPayloadSize)
            return std::nullopt;
        if (len != PacketHeader::kSize + hdr->payloadSize + PacketHeader::kCrcSize)
            return std::nullopt;
        const std::size_t payloadOffset = PacketHeader::kSize;
        const std::size_t crcOffset     = payloadOffset + hdr->payloadSize;
        std::uint32_t transmittedCrc    = (static_cast<std::uint32_t>(data[crcOffset]) << 24) |
                                       (static_cast<std::uint32_t>(data[crcOffset + 1]) << 16) |
                                       (static_cast<std::uint32_t>(data[crcOffset + 2]) << 8) |
                                       static_cast<std::uint32_t>(data[crcOffset + 3]);
        auto computedCrc = PacketHeader::crc32(data, crcOffset);
        if (computedCrc != transmittedCrc)
            return std::nullopt;
        std::size_t o = payloadOffset;
        std::uint32_t eid{};
        eid = (static_cast<std::uint32_t>(data[o]) << 24) | (static_cast<std::uint32_t>(data[o + 1]) << 16) |
              (static_cast<std::uint32_t>(data[o + 2]) << 8) | static_cast<std::uint32_t>(data[o + 3]);
        o += 4;
        std::uint8_t etype = data[o];
        o += 1;
        std::uint32_t bx{};
        std::uint32_t by{};
        bx = (static_cast<std::uint32_t>(data[o]) << 24) | (static_cast<std::uint32_t>(data[o + 1]) << 16) |
             (static_cast<std::uint32_t>(data[o + 2]) << 8) | static_cast<std::uint32_t>(data[o + 3]);
        o += 4;
        by = (static_cast<std::uint32_t>(data[o]) << 24) | (static_cast<std::uint32_t>(data[o + 1]) << 16) |
             (static_cast<std::uint32_t>(data[o + 2]) << 8) | static_cast<std::uint32_t>(data[o + 3]);
        float px = std::bit_cast<float>(bx);
        float py = std::bit_cast<float>(by);
        if (!std::isfinite(px) || !std::isfinite(py))
            return std::nullopt;
        EntitySpawnPacket p{};
        p.header     = *hdr;
        p.entityId   = eid;
        p.entityType = etype;
        p.posX       = px;
        p.posY       = py;
        return p;
    }
};

static_assert(EntitySpawnPacket::kSize == 32, "EntitySpawnPacket wire size must remain 32 bytes");
