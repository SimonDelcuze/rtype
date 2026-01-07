#pragma once

#include "network/PacketHeader.hpp"

#include <array>
#include <cstdint>
#include <optional>

struct DesyncDetectedPacket
{
    PacketHeader header{};
    std::uint32_t playerId = 0;
    std::uint64_t tick = 0;
    std::uint8_t desyncType = 0;
    std::uint32_t expectedChecksum = 0;
    std::uint32_t actualChecksum = 0;

    static constexpr std::size_t kPayloadSize = 4 + 8 + 1 + 4 + 4;
    static constexpr std::size_t kSize = PacketHeader::kSize + kPayloadSize + PacketHeader::kCrcSize;

    [[nodiscard]] std::array<std::uint8_t, kSize> encode() const noexcept
    {
        PacketHeader h = header;
        h.version      = PacketHeader::kProtocolVersion;
        h.messageType  = static_cast<std::uint8_t>(MessageType::DesyncDetected);
        h.payloadSize  = static_cast<std::uint16_t>(kPayloadSize);
        auto hdr       = h.encode();

        std::array<std::uint8_t, kSize> out{};
        for (std::size_t i = 0; i < hdr.size(); ++i)
            out[i] = hdr[i];

        std::size_t o = PacketHeader::kSize;
        auto w32 = [&](std::uint32_t v) {
            out[o]     = static_cast<std::uint8_t>((v >> 24) & 0xFF);
            out[o + 1] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
            out[o + 2] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
            out[o + 3] = static_cast<std::uint8_t>(v & 0xFF);
            o += 4;
        };
        auto w64 = [&](std::uint64_t v) {
            out[o]     = static_cast<std::uint8_t>((v >> 56) & 0xFF);
            out[o + 1] = static_cast<std::uint8_t>((v >> 48) & 0xFF);
            out[o + 2] = static_cast<std::uint8_t>((v >> 40) & 0xFF);
            out[o + 3] = static_cast<std::uint8_t>((v >> 32) & 0xFF);
            out[o + 4] = static_cast<std::uint8_t>((v >> 24) & 0xFF);
            out[o + 5] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
            out[o + 6] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
            out[o + 7] = static_cast<std::uint8_t>(v & 0xFF);
            o += 8;
        };

        w32(playerId);
        w64(tick);
        out[o++] = desyncType;
        w32(expectedChecksum);
        w32(actualChecksum);

        auto crc   = PacketHeader::crc32(out.data(), PacketHeader::kSize + kPayloadSize);
        out[o]     = static_cast<std::uint8_t>((crc >> 24) & 0xFF);
        out[o + 1] = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
        out[o + 2] = static_cast<std::uint8_t>((crc >> 8) & 0xFF);
        out[o + 3] = static_cast<std::uint8_t>(crc & 0xFF);

        return out;
    }

    [[nodiscard]] static std::optional<DesyncDetectedPacket> decode(const std::uint8_t* data,
                                                                     std::size_t len) noexcept
    {
        if (data == nullptr || len < kSize)
            return std::nullopt;

        auto hdr = PacketHeader::decode(data, len);
        if (!hdr)
            return std::nullopt;
        if (hdr->messageType != static_cast<std::uint8_t>(MessageType::DesyncDetected))
            return std::nullopt;
        if (hdr->payloadSize != kPayloadSize)
            return std::nullopt;
        if (len != PacketHeader::kSize + hdr->payloadSize + PacketHeader::kCrcSize)
            return std::nullopt;

        const std::size_t payloadOffset = PacketHeader::kSize;
        const std::size_t crcOffset     = payloadOffset + hdr->payloadSize;
        std::uint32_t transmittedCrc = (static_cast<std::uint32_t>(data[crcOffset]) << 24) |
                                       (static_cast<std::uint32_t>(data[crcOffset + 1]) << 16) |
                                       (static_cast<std::uint32_t>(data[crcOffset + 2]) << 8) |
                                       static_cast<std::uint32_t>(data[crcOffset + 3]);
        auto computedCrc = PacketHeader::crc32(data, crcOffset);
        if (computedCrc != transmittedCrc)
            return std::nullopt;

        std::size_t o = PacketHeader::kSize;
        auto r32 = [&](std::uint32_t& v) {
            v = (static_cast<std::uint32_t>(data[o]) << 24) |
                (static_cast<std::uint32_t>(data[o + 1]) << 16) |
                (static_cast<std::uint32_t>(data[o + 2]) << 8) |
                static_cast<std::uint32_t>(data[o + 3]);
            o += 4;
        };
        auto r64 = [&](std::uint64_t& v) {
            v = (static_cast<std::uint64_t>(data[o]) << 56) |
                (static_cast<std::uint64_t>(data[o + 1]) << 48) |
                (static_cast<std::uint64_t>(data[o + 2]) << 40) |
                (static_cast<std::uint64_t>(data[o + 3]) << 32) |
                (static_cast<std::uint64_t>(data[o + 4]) << 24) |
                (static_cast<std::uint64_t>(data[o + 5]) << 16) |
                (static_cast<std::uint64_t>(data[o + 6]) << 8) |
                static_cast<std::uint64_t>(data[o + 7]);
            o += 8;
        };

        DesyncDetectedPacket p{};
        p.header = *hdr;
        r32(p.playerId);
        r64(p.tick);
        p.desyncType = data[o++];
        r32(p.expectedChecksum);
        r32(p.actualChecksum);

        return p;
    }
};

static_assert(DesyncDetectedPacket::kSize == 42, "DesyncDetectedPacket wire size must remain 42 bytes");
