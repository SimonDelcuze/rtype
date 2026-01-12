#pragma once

#include "network/PacketHeader.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

class GameEndPacket
{
  public:
    inline static std::vector<std::uint8_t> create(bool victory, std::int32_t finalScore)
    {
        PacketHeader header;
        header.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
        header.messageType  = static_cast<std::uint8_t>(MessageType::GameEnd);
        header.payloadSize  = 5;
        header.originalSize = 5;

        std::vector<std::uint8_t> payload;
        payload.reserve(5);
        payload.push_back(victory ? 1 : 0);
        payload.push_back(static_cast<std::uint8_t>((finalScore >> 24) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>((finalScore >> 16) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>((finalScore >> 8) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>(finalScore & 0xFF));

        auto headerBytes = header.encode();
        std::vector<std::uint8_t> packet(headerBytes.begin(), headerBytes.end());
        packet.insert(packet.end(), payload.begin(), payload.end());

        auto crc = PacketHeader::crc32(packet.data(), packet.size());
        packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

        return packet;
    }

    static std::optional<GameEndPacket> decode(const std::uint8_t* data, std::size_t len)
    {
        if (len < PacketHeader::kSize + 5 + PacketHeader::kCrcSize) {
            return std::nullopt;
        }
        auto header = PacketHeader::decode(data, len);
        if (!header || header->messageType != static_cast<std::uint8_t>(MessageType::GameEnd)) {
            return std::nullopt;
        }

        GameEndPacket pkt;
        std::size_t offset = PacketHeader::kSize;
        pkt.victory        = data[offset] != 0;
        offset++;
        pkt.finalScore =
            (static_cast<std::int32_t>(data[offset]) << 24) | (static_cast<std::int32_t>(data[offset + 1]) << 16) |
            (static_cast<std::int32_t>(data[offset + 2]) << 8) | static_cast<std::int32_t>(data[offset + 3]);
        return pkt;
    }

    bool victory;
    std::int32_t finalScore;
};
