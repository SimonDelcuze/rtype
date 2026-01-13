#pragma once

#include "network/PacketHeader.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

struct PlayerScore
{
    std::uint32_t playerId;
    std::int32_t score;
};

class GameEndPacket
{
  public:
    inline static std::vector<std::uint8_t> create(bool victory, const std::vector<PlayerScore>& playerScores)
    {
        PacketHeader header;
        header.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        header.messageType = static_cast<std::uint8_t>(MessageType::GameEnd);

        std::uint8_t playerCount =
            static_cast<std::uint8_t>(std::min(playerScores.size(), static_cast<std::size_t>(255)));
        std::size_t payloadSize = 2 + (playerCount * 8);
        header.payloadSize      = static_cast<std::uint16_t>(payloadSize);
        header.originalSize     = static_cast<std::uint16_t>(payloadSize);

        std::vector<std::uint8_t> payload;
        payload.reserve(payloadSize);
        payload.push_back(victory ? 1 : 0);
        payload.push_back(playerCount);

        for (std::size_t i = 0; i < playerCount; ++i) {
            const auto& ps = playerScores[i];
            payload.push_back(static_cast<std::uint8_t>((ps.playerId >> 24) & 0xFF));
            payload.push_back(static_cast<std::uint8_t>((ps.playerId >> 16) & 0xFF));
            payload.push_back(static_cast<std::uint8_t>((ps.playerId >> 8) & 0xFF));
            payload.push_back(static_cast<std::uint8_t>(ps.playerId & 0xFF));
            payload.push_back(static_cast<std::uint8_t>((ps.score >> 24) & 0xFF));
            payload.push_back(static_cast<std::uint8_t>((ps.score >> 16) & 0xFF));
            payload.push_back(static_cast<std::uint8_t>((ps.score >> 8) & 0xFF));
            payload.push_back(static_cast<std::uint8_t>(ps.score & 0xFF));
        }

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
        if (len < PacketHeader::kSize + 2 + PacketHeader::kCrcSize) {
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

        std::uint8_t playerCount = data[offset];
        offset++;

        if (len < PacketHeader::kSize + 2 + (playerCount * 8) + PacketHeader::kCrcSize) {
            return std::nullopt;
        }

        pkt.playerScores.reserve(playerCount);
        for (std::uint8_t i = 0; i < playerCount; ++i) {
            PlayerScore ps;
            ps.playerId = (static_cast<std::uint32_t>(data[offset]) << 24) |
                          (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
                          (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
                          static_cast<std::uint32_t>(data[offset + 3]);
            offset += 4;
            ps.score = (static_cast<std::int32_t>(data[offset]) << 24) |
                       (static_cast<std::int32_t>(data[offset + 1]) << 16) |
                       (static_cast<std::int32_t>(data[offset + 2]) << 8) | static_cast<std::int32_t>(data[offset + 3]);
            offset += 4;
            pkt.playerScores.push_back(ps);
        }

        return pkt;
    }

    bool victory;
    std::vector<PlayerScore> playerScores;
};
