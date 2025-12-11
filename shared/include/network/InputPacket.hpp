#pragma once

#include "network/PacketHeader.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <optional>

enum class InputFlag : std::uint16_t
{
    None       = 0,
    MoveUp     = 1 << 0,
    MoveDown   = 1 << 1,
    MoveLeft   = 1 << 2,
    MoveRight  = 1 << 3,
    Fire       = 1 << 4,
    Charge1    = 1 << 5,
    Charge2    = 1 << 6,
    Charge3    = 1 << 7,
    Charge4    = 1 << 8,
    Charge5    = 1 << 9,
    ChargeMask = Charge1 | Charge2 | Charge3 | Charge4 | Charge5
};

constexpr InputFlag operator|(InputFlag a, InputFlag b)
{
    return static_cast<InputFlag>(static_cast<std::uint16_t>(a) | static_cast<std::uint16_t>(b));
}

constexpr InputFlag operator&(InputFlag a, InputFlag b)
{
    return static_cast<InputFlag>(static_cast<std::uint16_t>(a) & static_cast<std::uint16_t>(b));
}

constexpr InputFlag operator~(InputFlag f)
{
    return static_cast<InputFlag>(~static_cast<std::uint16_t>(f));
}

struct InputPacket
{
    PacketHeader header{};
    std::uint32_t playerId = 0;
    std::uint16_t flags    = 0;
    float x                = 0.0F;
    float y                = 0.0F;
    float angle            = 0.0F;

    static constexpr std::size_t kPayloadSize = 4 + 2 + 4 + 4 + 4;
    static constexpr std::size_t kSize        = PacketHeader::kSize + kPayloadSize + PacketHeader::kCrcSize;

    [[nodiscard]] std::array<std::uint8_t, kSize> encode() const noexcept
    {
        PacketHeader h = header;
        h.version      = PacketHeader::kProtocolVersion;
        h.packetType   = static_cast<std::uint8_t>(PacketType::ClientToServer);
        h.messageType  = static_cast<std::uint8_t>(MessageType::Input);
        h.payloadSize  = static_cast<std::uint16_t>(kPayloadSize);
        auto hdr       = h.encode();
        std::array<std::uint8_t, kSize> out{};
        for (std::size_t i = 0; i < hdr.size(); ++i)
            out[i] = hdr[i];
        std::size_t o = PacketHeader::kSize;
        auto w16      = [&](std::uint16_t v) {
            out[o]     = static_cast<std::uint8_t>((v >> 8) & 0xFF);
            out[o + 1] = static_cast<std::uint8_t>(v & 0xFF);
            o += 2;
        };
        auto w32 = [&](std::uint32_t v) {
            out[o]     = static_cast<std::uint8_t>((v >> 24) & 0xFF);
            out[o + 1] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
            out[o + 2] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
            out[o + 3] = static_cast<std::uint8_t>(v & 0xFF);
            o += 4;
        };
        w32(playerId);
        w16(flags);
        w32(std::bit_cast<std::uint32_t>(x));
        w32(std::bit_cast<std::uint32_t>(y));
        w32(std::bit_cast<std::uint32_t>(angle));
        auto crc   = PacketHeader::crc32(out.data(), PacketHeader::kSize + kPayloadSize);
        out[o]     = static_cast<std::uint8_t>((crc >> 24) & 0xFF);
        out[o + 1] = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
        out[o + 2] = static_cast<std::uint8_t>((crc >> 8) & 0xFF);
        out[o + 3] = static_cast<std::uint8_t>(crc & 0xFF);
        return out;
    }

    [[nodiscard]] static std::optional<InputPacket> decode(const std::uint8_t* data, std::size_t len) noexcept
    {
        if (data == nullptr || len < kSize)
            return std::nullopt;
        auto hdr = PacketHeader::decode(data, len);
        if (!hdr)
            return std::nullopt;
        if (hdr->messageType != static_cast<std::uint8_t>(MessageType::Input))
            return std::nullopt;
        if (hdr->packetType != static_cast<std::uint8_t>(PacketType::ClientToServer))
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
        std::size_t o = PacketHeader::kSize;
        auto r16      = [&](std::uint16_t& v) {
            v = static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[o]) << 8) |
                                                static_cast<std::uint16_t>(data[o + 1]));
            o += 2;
        };
        auto r32 = [&](std::uint32_t& v) {
            v = (static_cast<std::uint32_t>(data[o]) << 24) | (static_cast<std::uint32_t>(data[o + 1]) << 16) |
                (static_cast<std::uint32_t>(data[o + 2]) << 8) | static_cast<std::uint32_t>(data[o + 3]);
            o += 4;
        };
        InputPacket p{};
        p.header = *hdr;
        r32(p.playerId);
        r16(p.flags);
        std::uint32_t bx{};
        std::uint32_t by{};
        std::uint32_t ba{};
        r32(bx);
        r32(by);
        r32(ba);
        p.x     = std::bit_cast<float>(bx);
        p.y     = std::bit_cast<float>(by);
        p.angle = std::bit_cast<float>(ba);
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.angle))
            return std::nullopt;
        return p;
    }
};

static_assert(InputPacket::kSize == 37, "InputPacket wire size must remain 37 bytes");
