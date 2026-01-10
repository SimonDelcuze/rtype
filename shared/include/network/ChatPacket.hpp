#pragma once

#include "network/PacketHeader.hpp"

#include <cstring>
#include <optional>
#include <string>
#include <vector>

struct ChatPacket
{
    std::uint32_t roomId{0};
    std::uint32_t playerId{0};
    char playerName[32]{0};
    char message[121]{0};

    [[nodiscard]] std::vector<std::uint8_t> encode(std::uint16_t sequence) const
    {
        PacketHeader hdr{};
        hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        hdr.messageType = static_cast<std::uint8_t>(MessageType::Chat);
        hdr.sequenceId  = sequence;
        hdr.payloadSize = sizeof(roomId) + sizeof(playerId) + 32 + 121;

        auto encodedHdr = hdr.encode();
        std::vector<std::uint8_t> packet(encodedHdr.begin(), encodedHdr.end());

        // Room ID
        packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

        // Player ID
        packet.push_back(static_cast<std::uint8_t>((playerId >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((playerId >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((playerId >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(playerId & 0xFF));

        // Player Name
        for (int i = 0; i < 32; ++i)
            packet.push_back(static_cast<std::uint8_t>(playerName[i]));

        // Message
        for (int i = 0; i < 121; ++i)
            packet.push_back(static_cast<std::uint8_t>(message[i]));

        auto crc = PacketHeader::crc32(packet.data(), packet.size());
        packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

        return packet;
    }

    static std::optional<ChatPacket> decode(const std::uint8_t* data, std::size_t size)
    {
        if (size < PacketHeader::kSize + sizeof(std::uint32_t) * 2 + 32 + 121 + PacketHeader::kCrcSize)
            return std::nullopt;

        ChatPacket pkt;
        const std::uint8_t* ptr = data + PacketHeader::kSize;

        pkt.roomId = (static_cast<std::uint32_t>(ptr[0]) << 24) | (static_cast<std::uint32_t>(ptr[1]) << 16) |
                     (static_cast<std::uint32_t>(ptr[2]) << 8) | static_cast<std::uint32_t>(ptr[3]);
        ptr += 4;

        pkt.playerId = (static_cast<std::uint32_t>(ptr[0]) << 24) | (static_cast<std::uint32_t>(ptr[1]) << 16) |
                       (static_cast<std::uint32_t>(ptr[2]) << 8) | static_cast<std::uint32_t>(ptr[3]);
        ptr += 4;

        std::memcpy(pkt.playerName, ptr, 32);
        ptr += 32;

        std::memcpy(pkt.message, ptr, 121);

        return pkt;
    }
};
