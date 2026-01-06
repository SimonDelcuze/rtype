#pragma once

#include "network/PacketHeader.hpp"

#include <array>
#include <cstdint>
#include <optional>

struct EntityDestroyedPacket
{
    PacketHeader header{};
    std::uint32_t entityId = 0;

    static constexpr std::size_t kPayloadSize = 4;
    static constexpr std::size_t kSize        = PacketHeader::kSize + kPayloadSize + PacketHeader::kCrcSize;

    [[nodiscard]] std::array<std::uint8_t, kSize> encode() const noexcept
    {
        PacketHeader h = header;
        h.version      = PacketHeader::kProtocolVersion;
        h.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
        h.messageType  = static_cast<std::uint8_t>(MessageType::EntityDestroyed);
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
        auto crc   = PacketHeader::crc32(out.data(), PacketHeader::kSize + kPayloadSize);
        out[o]     = static_cast<std::uint8_t>((crc >> 24) & 0xFF);
        out[o + 1] = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
        out[o + 2] = static_cast<std::uint8_t>((crc >> 8) & 0xFF);
        out[o + 3] = static_cast<std::uint8_t>(crc & 0xFF);
        return out;
    }

    [[nodiscard]] static std::optional<EntityDestroyedPacket> decode(const std::uint8_t* data, std::size_t len) noexcept
    {
        if (data == nullptr || len < kSize)
            return std::nullopt;
        auto hdr = PacketHeader::decode(data, len);
        if (!hdr)
            return std::nullopt;
        if (hdr->messageType != static_cast<std::uint8_t>(MessageType::EntityDestroyed))
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
        EntityDestroyedPacket p{};
        p.header   = *hdr;
        p.entityId = eid;
        return p;
    }
};

static_assert(EntityDestroyedPacket::kSize == 25, "EntityDestroyedPacket wire size must remain 25 bytes");
