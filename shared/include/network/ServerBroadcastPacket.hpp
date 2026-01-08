#pragma once

#include "network/PacketHeader.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>

struct ServerBroadcastPacket
{
    PacketHeader header{};
    std::array<char, 128> message{};

    static constexpr std::size_t kPayloadSize = 128;
    static constexpr std::size_t kSize        = PacketHeader::kSize + kPayloadSize + PacketHeader::kCrcSize;

    [[nodiscard]] std::array<std::uint8_t, kSize> encode() const noexcept
    {
        PacketHeader h = header;
        h.version      = PacketHeader::kProtocolVersion;
        h.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
        h.messageType  = static_cast<std::uint8_t>(MessageType::ServerBroadcast);
        h.payloadSize  = static_cast<std::uint16_t>(kPayloadSize);
        auto hdr       = h.encode();
        std::array<std::uint8_t, kSize> out{};
        for (std::size_t i = 0; i < hdr.size(); ++i)
            out[i] = hdr[i];
        std::memcpy(out.data() + PacketHeader::kSize, message.data(), kPayloadSize);
        auto crc      = PacketHeader::crc32(out.data(), PacketHeader::kSize + kPayloadSize);
        std::size_t o = PacketHeader::kSize + kPayloadSize;
        out[o]        = static_cast<std::uint8_t>((crc >> 24) & 0xFF);
        out[o + 1]    = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
        out[o + 2]    = static_cast<std::uint8_t>((crc >> 8) & 0xFF);
        out[o + 3]    = static_cast<std::uint8_t>(crc & 0xFF);
        return out;
    }

    [[nodiscard]] static std::optional<ServerBroadcastPacket> decode(const std::uint8_t* data, std::size_t len) noexcept
    {
        if (data == nullptr || len < kSize)
            return std::nullopt;
        auto hdr = PacketHeader::decode(data, len);
        if (!hdr)
            return std::nullopt;
        if (hdr->messageType != static_cast<std::uint8_t>(MessageType::ServerBroadcast))
            return std::nullopt;
        if (hdr->payloadSize != kPayloadSize)
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

        ServerBroadcastPacket p{};
        p.header = *hdr;
        std::memcpy(p.message.data(), data + payloadOffset, kPayloadSize);
        return p;
    }

    static ServerBroadcastPacket create(const std::string& msg)
    {
        ServerBroadcastPacket p{};
        std::size_t len = std::min(msg.size(), kPayloadSize - 1);
        std::memcpy(p.message.data(), msg.c_str(), len);
        p.message[len] = '\0';
        return p;
    }

    std::string getMessage() const
    {
        return std::string(message.data());
    }
};
