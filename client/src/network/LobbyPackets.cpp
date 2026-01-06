#include "network/LobbyPackets.hpp"

#include "network/PacketHeader.hpp"

std::vector<std::uint8_t> buildListRoomsPacket(std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType   = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType  = static_cast<std::uint8_t>(MessageType::LobbyListRooms);
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

std::vector<std::uint8_t> buildCreateRoomPacket(std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType   = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType  = static_cast<std::uint8_t>(MessageType::LobbyCreateRoom);
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

std::vector<std::uint8_t> buildJoinRoomPacket(std::uint32_t roomId, std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyJoinRoom);
    hdr.sequenceId  = sequence;

    constexpr std::uint16_t payloadSize = sizeof(std::uint32_t);
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

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}

std::optional<RoomListResult> parseRoomListPacket(const std::uint8_t* data, std::size_t size)
{
    if (size < PacketHeader::kSize + sizeof(std::uint16_t)) {
        return std::nullopt;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;
    std::uint16_t roomCount = (static_cast<std::uint16_t>(payload[0]) << 8) | static_cast<std::uint16_t>(payload[1]);

    constexpr std::size_t roomInfoSize = sizeof(std::uint32_t) + sizeof(std::uint16_t) * 3 + sizeof(std::uint8_t);

    if (size < PacketHeader::kSize + sizeof(std::uint16_t) + (roomCount * roomInfoSize) + PacketHeader::kCrcSize) {
        return std::nullopt;
    }

    RoomListResult result;
    result.rooms.reserve(roomCount);

    const std::uint8_t* ptr = payload + sizeof(std::uint16_t);

    for (std::uint16_t i = 0; i < roomCount; ++i) {
        RoomInfo info;

        info.roomId = (static_cast<std::uint32_t>(ptr[0]) << 24) | (static_cast<std::uint32_t>(ptr[1]) << 16) |
                      (static_cast<std::uint32_t>(ptr[2]) << 8) | static_cast<std::uint32_t>(ptr[3]);
        ptr += 4;

        info.playerCount = (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
        ptr += 2;

        info.maxPlayers = (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
        ptr += 2;

        info.port = (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
        ptr += 2;

        info.state = static_cast<RoomState>(ptr[0]);
        ptr += 1;

        result.rooms.push_back(info);
    }

    return result;
}

std::optional<RoomCreatedResult> parseRoomCreatedPacket(const std::uint8_t* data, std::size_t size)
{
    constexpr std::size_t expectedSize =
        PacketHeader::kSize + sizeof(std::uint32_t) + sizeof(std::uint16_t) + PacketHeader::kCrcSize;

    if (size < expectedSize) {
        return std::nullopt;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;

    RoomCreatedResult result;
    result.roomId = (static_cast<std::uint32_t>(payload[0]) << 24) | (static_cast<std::uint32_t>(payload[1]) << 16) |
                    (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);

    result.port = (static_cast<std::uint16_t>(payload[4]) << 8) | static_cast<std::uint16_t>(payload[5]);

    return result;
}

std::optional<JoinSuccessResult> parseJoinSuccessPacket(const std::uint8_t* data, std::size_t size)
{
    constexpr std::size_t expectedSize =
        PacketHeader::kSize + sizeof(std::uint32_t) + sizeof(std::uint16_t) + PacketHeader::kCrcSize;

    if (size < expectedSize) {
        return std::nullopt;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;

    JoinSuccessResult result;
    result.roomId = (static_cast<std::uint32_t>(payload[0]) << 24) | (static_cast<std::uint32_t>(payload[1]) << 16) |
                    (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);

    result.port = (static_cast<std::uint16_t>(payload[4]) << 8) | static_cast<std::uint16_t>(payload[5]);

    return result;
}
