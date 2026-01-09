#pragma once

#include "network/PacketHeader.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

// GetStatsRequest: Client requests their own stats
// Payload: Empty (uses authenticated session)
struct GetStatsRequestData
{
    // No payload needed - uses JWT from authenticated session
};

// GetStatsResponse: Server sends user stats
// Payload format:
// - userId (4 bytes)
// - gamesPlayed (4 bytes)
// - wins (4 bytes)
// - losses (4 bytes)
// - totalScore (8 bytes)
struct GetStatsResponseData
{
    std::uint32_t userId;
    std::uint32_t gamesPlayed;
    std::uint32_t wins;
    std::uint32_t losses;
    std::uint64_t totalScore;
};

// Build GetStatsRequest packet
inline std::vector<std::uint8_t> buildGetStatsRequestPacket(std::uint16_t sequenceId = 0)
{
    PacketHeader header;
    header.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    header.messageType = static_cast<std::uint8_t>(MessageType::AuthGetStatsRequest);
    header.sequenceId  = sequenceId;
    header.payloadSize = 0; // No payload

    auto headerBytes = header.encode();
    std::vector<std::uint8_t> packet(headerBytes.begin(), headerBytes.end());

    // Add CRC32 placeholder (will be calculated by network layer)
    packet.push_back(0);
    packet.push_back(0);
    packet.push_back(0);
    packet.push_back(0);

    return packet;
}

// Build GetStatsResponse packet
inline std::vector<std::uint8_t> buildGetStatsResponsePacket(const GetStatsResponseData& stats,
                                                             std::uint16_t sequenceId = 0)
{
    PacketHeader header;
    header.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    header.messageType = static_cast<std::uint8_t>(MessageType::AuthGetStatsResponse);
    header.sequenceId  = sequenceId;
    header.payloadSize = 24; // 4 + 4 + 4 + 4 + 8 = 24 bytes

    auto headerBytes = header.encode();
    std::vector<std::uint8_t> packet(headerBytes.begin(), headerBytes.end());

    // Add payload
    // userId (4 bytes)
    packet.push_back(static_cast<std::uint8_t>((stats.userId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.userId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.userId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(stats.userId & 0xFF));

    // gamesPlayed (4 bytes)
    packet.push_back(static_cast<std::uint8_t>((stats.gamesPlayed >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.gamesPlayed >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.gamesPlayed >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(stats.gamesPlayed & 0xFF));

    // wins (4 bytes)
    packet.push_back(static_cast<std::uint8_t>((stats.wins >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.wins >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.wins >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(stats.wins & 0xFF));

    // losses (4 bytes)
    packet.push_back(static_cast<std::uint8_t>((stats.losses >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.losses >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.losses >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(stats.losses & 0xFF));

    // totalScore (8 bytes)
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 56) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 48) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 40) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 32) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((stats.totalScore >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(stats.totalScore & 0xFF));

    // Add CRC32 placeholder
    packet.push_back(0);
    packet.push_back(0);
    packet.push_back(0);
    packet.push_back(0);

    return packet;
}

// Parse GetStatsResponse packet
inline std::optional<GetStatsResponseData> parseGetStatsResponsePacket(const std::uint8_t* data, std::size_t size)
{
    // Skip the packet header (8 bytes) to get to the payload
    if (size < PacketHeader::kSize + 24) {
        return std::nullopt;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;

    GetStatsResponseData stats;

    // Parse userId (big-endian)
    stats.userId = (static_cast<std::uint32_t>(payload[0]) << 24) | (static_cast<std::uint32_t>(payload[1]) << 16) |
                   (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);

    // Parse gamesPlayed (big-endian)
    stats.gamesPlayed = (static_cast<std::uint32_t>(payload[4]) << 24) |
                        (static_cast<std::uint32_t>(payload[5]) << 16) | (static_cast<std::uint32_t>(payload[6]) << 8) |
                        static_cast<std::uint32_t>(payload[7]);

    // Parse wins (big-endian)
    stats.wins = (static_cast<std::uint32_t>(payload[8]) << 24) | (static_cast<std::uint32_t>(payload[9]) << 16) |
                 (static_cast<std::uint32_t>(payload[10]) << 8) | static_cast<std::uint32_t>(payload[11]);

    // Parse losses (big-endian)
    stats.losses = (static_cast<std::uint32_t>(payload[12]) << 24) | (static_cast<std::uint32_t>(payload[13]) << 16) |
                   (static_cast<std::uint32_t>(payload[14]) << 8) | static_cast<std::uint32_t>(payload[15]);

    // Parse totalScore (big-endian)
    stats.totalScore =
        (static_cast<std::uint64_t>(payload[16]) << 56) | (static_cast<std::uint64_t>(payload[17]) << 48) |
        (static_cast<std::uint64_t>(payload[18]) << 40) | (static_cast<std::uint64_t>(payload[19]) << 32) |
        (static_cast<std::uint64_t>(payload[20]) << 24) | (static_cast<std::uint64_t>(payload[21]) << 16) |
        (static_cast<std::uint64_t>(payload[22]) << 8) | static_cast<std::uint64_t>(payload[23]);

    return stats;
}
