#pragma once

#include "network/PacketHeader.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>

struct ServerDisconnectPacket
{
    PacketHeader header{};
    std::array<char, 64> reason{};

    static constexpr std::size_t kPayloadSize = 64;
    static constexpr std::size_t kSize        = PacketHeader::kSize + kPayloadSize + PacketHeader::kCrcSize;

    [[nodiscard]] std::array<std::uint8_t, kSize> encode() const noexcept
    {
        PacketHeader h = header;
        h.version      = PacketHeader::kProtocolVersion;
        h.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
        h.messageType  = static_cast<std::uint8_t>(MessageType::ServerDisconnect);
        h.payloadSize  = static_cast<std::uint16_t>(kPayloadSize);
        auto hdr       = h.encode();
        std::array<std::uint8_t, kSize> out{};
        for (std::size_t i = 0; i < hdr.size(); ++i)
            out[i] = hdr[i];
        std::memcpy(out.data() + PacketHeader::kSize, reason.data(), kPayloadSize);
        auto crc   = PacketHeader::crc32(out.data(), PacketHeader::kSize + kPayloadSize);
        std::size_t o = PacketHeader::kSize + kPayloadSize;
        out[o]     = static_cast<std::uint8_t>((crc >> 24) & 0xFF);
        out[o + 1] = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
        out[o + 2] = static_cast<std::uint8_t>((crc >> 8) & 0xFF);
        out[o + 3] = static_cast<std::uint8_t>(crc & 0xFF);
        return out;
    }

    [[nodiscard]] static std::optional<ServerDisconnectPacket> decode(const std::uint8_t* data,
                                                                      std::size_t len) noexcept
    {
        if (data == nullptr || len < kSize)
            return std::nullopt;
        auto hdr = PacketHeader::decode(data, len);
        if (!hdr)
            return std::nullopt;
        if (hdr->messageType != static_cast<std::uint8_t>(MessageType::ServerDisconnect) &&
            hdr->messageType != static_cast<std::uint8_t>(MessageType::ServerKick) &&
            hdr->messageType != static_cast<std::uint8_t>(MessageType::ServerBan))
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
        ServerDisconnectPacket p{};
        p.header = *hdr;
        std::memcpy(p.reason.data(), data + payloadOffset, kPayloadSize);
        return p;
    }

    static ServerDisconnectPacket create(const std::string& reasonMsg, MessageType type = MessageType::ServerDisconnect)
    {
        ServerDisconnectPacket p{};
        p.header.messageType = static_cast<std::uint8_t>(type);
        std::size_t len = std::min(reasonMsg.size(), kPayloadSize - 1);
        std::memcpy(p.reason.data(), reasonMsg.c_str(), len);
        p.reason[len] = '\0';
        return p;
    }

    std::string getReason() const
    {
        return std::string(reason.data());
    }
};

static_assert(ServerDisconnectPacket::kSize == 17 + 64 + 4, "ServerDisconnectPacket wire size mismatch");
