#pragma once

#include "network/PacketHeader.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

struct GetStatsRequestData
{};

struct GetStatsResponseData
{
    std::uint32_t userId;
    char username[32];
    std::uint32_t gamesPlayed;
    std::uint32_t wins;
    std::uint32_t losses;
    std::uint64_t totalScore;
};

inline std::vector<std::uint8_t> buildGetStatsRequestPacket(std::uint16_t sequenceId = 0)
{
    PacketHeader header;
    header.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    header.messageType = static_cast<std::uint8_t>(MessageType::AuthGetStatsRequest);
    header.sequenceId  = sequenceId;
    header.payloadSize = 0;

    auto headerBytes = header.encode();
    std::vector<std::uint8_t> packet(headerBytes.begin(), headerBytes.end());

    packet.push_back(0);
    packet.push_back(0);
    packet.push_back(0);
    packet.push_back(0);

    return packet;
}

inline std::vector<std::uint8_t> buildGetStatsResponsePacket(const GetStatsResponseData& stats,
                                                             std::uint16_t sequenceId = 0)
{
    PacketHeader header;
    header.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    header.messageType = static_cast<std::uint8_t>(MessageType::AuthGetStatsResponse);
    header.sequenceId  = sequenceId;
    header.payloadSize = 56;

    auto headerBytes = header.encode();
    std::vector<std::uint8_t> packet(headerBytes.begin(), headerBytes.end());

    packet.push_back(static_cast<std::uint8_t>((stats.userId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.userId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.userId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(stats.userId & 0xFF));

    for (int i = 0; i < 32; ++i) {
        if (i < 32) {
            packet.push_back(static_cast<std::uint8_t>(stats.username[i]));
        } else {
            packet.push_back(0);
        }
    }

    packet.push_back(static_cast<std::uint8_t>((stats.gamesPlayed >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.gamesPlayed >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.gamesPlayed >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(stats.gamesPlayed & 0xFF));

    packet.push_back(static_cast<std::uint8_t>((stats.wins >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.wins >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.wins >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(stats.wins & 0xFF));

    packet.push_back(static_cast<std::uint8_t>((stats.losses >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.losses >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.losses >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(stats.losses & 0xFF));

    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 56) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 48) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 40) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 32) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(stats.totalScore & 0xFF));

    packet.push_back(0);
    packet.push_back(0);
    packet.push_back(0);
    packet.push_back(0);

    return packet;
}

inline std::optional<GetStatsResponseData> parseGetStatsResponsePacket(const std::uint8_t* data, std::size_t size)
{
    if (size < PacketHeader::kSize + 56) {
        return std::nullopt;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;

    GetStatsResponseData stats;

    stats.userId = (static_cast<std::uint32_t>(payload[0]) << 24) | (static_cast<std::uint32_t>(payload[1]) << 16) |
                   (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);
    payload += 4;

    std::memcpy(stats.username, payload, 32);
    payload += 32;

    stats.gamesPlayed = (static_cast<std::uint32_t>(payload[4]) << 24) |
                        (static_cast<std::uint32_t>(payload[5]) << 16) | (static_cast<std::uint32_t>(payload[6]) << 8) |
                        static_cast<std::uint32_t>(payload[7]);

    stats.wins = (static_cast<std::uint32_t>(payload[8]) << 24) | (static_cast<std::uint32_t>(payload[9]) << 16) |
                 (static_cast<std::uint32_t>(payload[10]) << 8) | static_cast<std::uint32_t>(payload[11]);

    stats.losses = (static_cast<std::uint32_t>(payload[12]) << 24) | (static_cast<std::uint32_t>(payload[13]) << 16) |
                   (static_cast<std::uint32_t>(payload[14]) << 8) | static_cast<std::uint32_t>(payload[15]);

    stats.totalScore =
        (static_cast<std::uint64_t>(payload[16]) << 56) | (static_cast<std::uint64_t>(payload[17]) << 48) |
        (static_cast<std::uint64_t>(payload[18]) << 40) | (static_cast<std::uint64_t>(payload[19]) << 32) |
        (static_cast<std::uint64_t>(payload[20]) << 24) | (static_cast<std::uint64_t>(payload[21]) << 16) |
        (static_cast<std::uint64_t>(payload[22]) << 8) | static_cast<std::uint64_t>(payload[23]);

    return stats;
}
