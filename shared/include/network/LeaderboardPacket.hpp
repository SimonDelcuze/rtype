#pragma once

#include "network/PacketHeader.hpp"

#include <cstring>
#include <string>
#include <vector>

struct LeaderboardEntry
{
    char username[32];
    std::int32_t value;
};

struct LeaderboardResponseData
{
    std::vector<LeaderboardEntry> topElo;
    std::vector<LeaderboardEntry> topScore;
};

inline std::vector<std::uint8_t> buildLeaderboardRequestPacket(std::uint16_t sequenceId = 0)
{
    PacketHeader header;
    header.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    header.messageType = static_cast<std::uint8_t>(MessageType::LeaderboardRequest);
    header.sequenceId  = sequenceId;
    header.payloadSize = 0;

    auto encoded = header.encode();
    return std::vector<std::uint8_t>(encoded.begin(), encoded.end());
}

inline std::vector<std::uint8_t> buildLeaderboardResponsePacket(const LeaderboardResponseData& data,
                                                                std::uint16_t sequenceId = 0)
{
    PacketHeader header;
    header.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    header.messageType = static_cast<std::uint8_t>(MessageType::LeaderboardResponse);
    header.sequenceId  = sequenceId;

    header.payloadSize = static_cast<std::uint16_t>(2 + (data.topElo.size() + data.topScore.size()) * 36);

    auto headerBytes = header.encode();
    std::vector<std::uint8_t> packet(headerBytes.begin(), headerBytes.end());

    packet.push_back(static_cast<std::uint8_t>(data.topElo.size()));
    for (const auto& entry : data.topElo) {
        for (int i = 0; i < 32; ++i) {
            packet.push_back(static_cast<std::uint8_t>(entry.username[i]));
        }
        packet.push_back(static_cast<std::uint8_t>((entry.value >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((entry.value >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((entry.value >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(entry.value & 0xFF));
    }

    packet.push_back(static_cast<std::uint8_t>(data.topScore.size()));
    for (const auto& entry : data.topScore) {
        for (int i = 0; i < 32; ++i) {
            packet.push_back(static_cast<std::uint8_t>(entry.username[i]));
        }
        packet.push_back(static_cast<std::uint8_t>((entry.value >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((entry.value >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((entry.value >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(entry.value & 0xFF));
    }

    return packet;
}

inline LeaderboardResponseData parseLeaderboardResponsePacket(const std::uint8_t* data, std::size_t size)
{
    LeaderboardResponseData response;
    if (size < PacketHeader::kSize + 2)
        return response;

    const std::uint8_t* ptr = data + PacketHeader::kSize;
    std::uint8_t countElo   = *ptr++;
    for (int i = 0; i < countElo; ++i) {
        if (ptr + 36 > data + size)
            break;
        LeaderboardEntry entry;
        std::memcpy(entry.username, ptr, 32);
        ptr += 32;
        entry.value = (static_cast<std::uint32_t>(ptr[0]) << 24) | (static_cast<std::uint32_t>(ptr[1]) << 16) |
                      (static_cast<std::uint32_t>(ptr[2]) << 8) | static_cast<std::uint32_t>(ptr[3]);
        ptr += 4;
        response.topElo.push_back(entry);
    }

    if (ptr >= data + size)
        return response;
    std::uint8_t countScore = *ptr++;
    for (int i = 0; i < countScore; ++i) {
        if (ptr + 36 > data + size)
            break;
        LeaderboardEntry entry;
        std::memcpy(entry.username, ptr, 32);
        ptr += 32;
        entry.value = (static_cast<std::uint32_t>(ptr[0]) << 24) | (static_cast<std::uint32_t>(ptr[1]) << 16) |
                      (static_cast<std::uint32_t>(ptr[2]) << 8) | static_cast<std::uint32_t>(ptr[3]);
        ptr += 4;
        response.topScore.push_back(entry);
    }

    return response;
}
