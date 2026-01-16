#include "lobby/LobbyPackets.hpp"

#include <cstring>

std::vector<std::uint8_t> buildRoomListPacket(const std::vector<RoomInfo>& rooms, std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyRoomList);
    hdr.sequenceId  = sequence;

    std::uint16_t roomCount = static_cast<std::uint16_t>(rooms.size());

    std::vector<std::uint8_t> payload;
    constexpr std::size_t kFixedRoomSize = sizeof(std::uint32_t) + sizeof(std::uint8_t) + sizeof(std::uint16_t) * 3 +
                                           sizeof(std::uint8_t) * 4 + sizeof(std::uint32_t) + sizeof(std::uint16_t) * 2;
    std::size_t estimatedPayload = sizeof(std::uint16_t);
    for (const auto& room : rooms) {
        estimatedPayload += kFixedRoomSize + room.roomName.size() + room.inviteCode.size();
    }
    payload.reserve(estimatedPayload);

    payload.push_back(static_cast<std::uint8_t>((roomCount >> 8) & 0xFF));
    payload.push_back(static_cast<std::uint8_t>(roomCount & 0xFF));

    for (const auto& room : rooms) {
        payload.push_back(static_cast<std::uint8_t>((room.roomId >> 24) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>((room.roomId >> 16) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>((room.roomId >> 8) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>(room.roomId & 0xFF));

        payload.push_back(static_cast<std::uint8_t>(room.roomType));

        std::uint16_t playerCount = static_cast<std::uint16_t>(room.playerCount);
        payload.push_back(static_cast<std::uint8_t>((playerCount >> 8) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>(playerCount & 0xFF));

        std::uint16_t maxPlayers = static_cast<std::uint16_t>(room.maxPlayers);
        payload.push_back(static_cast<std::uint8_t>((maxPlayers >> 8) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>(maxPlayers & 0xFF));

        payload.push_back(static_cast<std::uint8_t>((room.port >> 8) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>(room.port & 0xFF));

        payload.push_back(static_cast<std::uint8_t>(room.state));

        payload.push_back(static_cast<std::uint8_t>((room.ownerId >> 24) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>((room.ownerId >> 16) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>((room.ownerId >> 8) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>(room.ownerId & 0xFF));

        payload.push_back(room.passwordProtected ? 1 : 0);

        payload.push_back(static_cast<std::uint8_t>(room.visibility));

        payload.push_back(room.countdown);

        std::uint16_t nameLen = static_cast<std::uint16_t>(room.roomName.size());
        payload.push_back(static_cast<std::uint8_t>((nameLen >> 8) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>(nameLen & 0xFF));
        payload.insert(payload.end(), room.roomName.begin(), room.roomName.end());

        std::uint16_t codeLen = static_cast<std::uint16_t>(room.inviteCode.size());
        payload.push_back(static_cast<std::uint8_t>((codeLen >> 8) & 0xFF));
        payload.push_back(static_cast<std::uint8_t>(codeLen & 0xFF));
        payload.insert(payload.end(), room.inviteCode.begin(), room.inviteCode.end());
    }

    hdr.payloadSize  = static_cast<std::uint16_t>(payload.size());
    hdr.originalSize = hdr.payloadSize;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + payload.size() + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());
    packet.insert(packet.end(), payload.begin(), payload.end());

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}

std::vector<std::uint8_t> buildRoomCreatedPacket(std::uint32_t roomId, std::uint16_t port, std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyRoomCreated);
    hdr.sequenceId  = sequence;

    constexpr std::uint16_t payloadSize = sizeof(std::uint32_t) + sizeof(std::uint16_t);
    hdr.payloadSize                     = payloadSize;
    hdr.originalSize                    = payloadSize;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    packet.push_back(static_cast<std::uint8_t>((port >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(port & 0xFF));

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}

std::vector<std::uint8_t> buildJoinSuccessPacket(std::uint32_t roomId, std::uint16_t port, std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyJoinSuccess);
    hdr.sequenceId  = sequence;

    constexpr std::uint16_t payloadSize = sizeof(std::uint32_t) + sizeof(std::uint16_t);
    hdr.payloadSize                     = payloadSize;
    hdr.originalSize                    = payloadSize;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    packet.push_back(static_cast<std::uint8_t>((port >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(port & 0xFF));

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}

std::vector<std::uint8_t> buildJoinFailedPacket(std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType  = static_cast<std::uint8_t>(MessageType::LobbyJoinFailed);
    hdr.sequenceId   = sequence;
    hdr.payloadSize  = 0;
    hdr.originalSize = 0;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}
