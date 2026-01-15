#include "network/LobbyPackets.hpp"

#include "network/PacketHeader.hpp"

#include <cstring>

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

std::vector<std::uint8_t> buildCreateRoomPacket(const std::string& roomName, const std::string& passwordHash,
                                                RoomVisibility visibility, std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyCreateRoom);
    hdr.sequenceId  = sequence;

    std::uint16_t payloadSize =
        sizeof(std::uint8_t) + sizeof(std::uint16_t) + roomName.size() + sizeof(std::uint16_t) + passwordHash.size();

    hdr.payloadSize  = payloadSize;
    hdr.originalSize = payloadSize;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    packet.push_back(static_cast<std::uint8_t>(visibility));

    std::uint16_t nameLen = static_cast<std::uint16_t>(roomName.size());
    packet.push_back(static_cast<std::uint8_t>((nameLen >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(nameLen & 0xFF));
    packet.insert(packet.end(), roomName.begin(), roomName.end());

    std::uint16_t passLen = static_cast<std::uint16_t>(passwordHash.size());
    packet.push_back(static_cast<std::uint8_t>((passLen >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(passLen & 0xFF));
    packet.insert(packet.end(), passwordHash.begin(), passwordHash.end());

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}

std::vector<std::uint8_t> buildJoinRoomPacket(std::uint32_t roomId, std::uint16_t sequence)
{
    return buildJoinRoomPacket(roomId, "", sequence);
}

std::vector<std::uint8_t> buildJoinRoomPacket(std::uint32_t roomId, const std::string& passwordHash,
                                              std::uint16_t sequence)
{
    extern bool g_joinAsSpectator;

    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyJoinRoom);
    hdr.sequenceId  = sequence;

    std::uint16_t payloadSize = sizeof(std::uint32_t) + sizeof(std::uint16_t) + passwordHash.size() + 1;
    hdr.payloadSize           = payloadSize;
    hdr.originalSize          = payloadSize;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    std::uint16_t passLen = static_cast<std::uint16_t>(passwordHash.size());
    packet.push_back(static_cast<std::uint8_t>((passLen >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(passLen & 0xFF));
    packet.insert(packet.end(), passwordHash.begin(), passwordHash.end());

    packet.push_back(g_joinAsSpectator ? 1 : 0);

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

    RoomListResult result;
    result.rooms.reserve(roomCount);

    const std::uint8_t* ptr = payload + sizeof(std::uint16_t);
    const std::uint8_t* end = data + size - PacketHeader::kCrcSize;

    for (std::uint16_t i = 0; i < roomCount; ++i) {
        if (ptr + 18 > end) {
            return std::nullopt;
        }

        RoomInfo info;

        info.roomId = (static_cast<std::uint32_t>(ptr[0]) << 24) | (static_cast<std::uint32_t>(ptr[1]) << 16) |
                      (static_cast<std::uint32_t>(ptr[2]) << 8) | static_cast<std::uint32_t>(ptr[3]);
        ptr += 4;

        info.roomType = static_cast<RoomType>(ptr[0]);
        ptr += 1;

        info.playerCount = (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
        ptr += 2;

        info.maxPlayers = (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
        ptr += 2;

        info.port = (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
        ptr += 2;

        info.state = static_cast<RoomState>(ptr[0]);
        ptr += 1;

        info.ownerId = (static_cast<std::uint32_t>(ptr[0]) << 24) | (static_cast<std::uint32_t>(ptr[1]) << 16) |
                       (static_cast<std::uint32_t>(ptr[2]) << 8) | static_cast<std::uint32_t>(ptr[3]);
        ptr += 4;

        info.passwordProtected = (ptr[0] != 0);
        ptr += 1;

        info.visibility = static_cast<RoomVisibility>(ptr[0]);
        ptr += 1;

        info.countdown = ptr[0];
        ptr += 1;

        if (ptr + 2 > end) {
            return std::nullopt;
        }
        std::uint16_t nameLen = (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
        ptr += 2;

        if (ptr + nameLen > end) {
            return std::nullopt;
        }
        info.roomName = std::string(reinterpret_cast<const char*>(ptr), nameLen);
        ptr += nameLen;

        if (ptr + 2 > end) {
            return std::nullopt;
        }
        std::uint16_t codeLen = (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
        ptr += 2;

        if (ptr + codeLen > end) {
            return std::nullopt;
        }
        info.inviteCode = std::string(reinterpret_cast<const char*>(ptr), codeLen);
        ptr += codeLen;

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

std::vector<std::uint8_t> buildGetPlayersPacket(std::uint32_t roomId, std::uint16_t sequence)
{
    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::RoomGetPlayers);
    hdr.sequenceId  = sequence;
    hdr.payloadSize = sizeof(std::uint32_t);

    auto encoded = hdr.encode();
    std::vector<std::uint8_t> packet(encoded.begin(), encoded.end());

    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    auto crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}

std::optional<std::vector<PlayerInfo>> parsePlayerListPacket(const std::uint8_t* data, std::size_t size)
{
    if (size < PacketHeader::kSize + sizeof(std::uint32_t) + sizeof(std::uint8_t) + PacketHeader::kCrcSize) {
        return std::nullopt;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;

    payload += 4;

    std::uint8_t countdown = payload[0];
    (void) countdown;
    payload += 1;

    std::uint8_t playerCount = payload[0];
    payload += 1;

    std::vector<PlayerInfo> players;
    players.reserve(playerCount);

    for (std::uint8_t i = 0; i < playerCount; ++i) {
        if (payload + 43 > data + size - PacketHeader::kCrcSize) {
            return std::nullopt;
        }

        PlayerInfo info;
        info.playerId = (static_cast<std::uint32_t>(payload[0]) << 24) |
                        (static_cast<std::uint32_t>(payload[1]) << 16) | (static_cast<std::uint32_t>(payload[2]) << 8) |
                        static_cast<std::uint32_t>(payload[3]);
        payload += 4;

        std::memcpy(info.name, payload, 32);
        payload += 32;

        info.isHost = (payload[0] != 0);
        payload += 1;

        info.isReady = (payload[0] != 0);
        payload += 1;

        info.isSpectator = (payload[0] != 0);
        payload += 1;

        info.elo = (static_cast<std::int32_t>(payload[0]) << 24) | (static_cast<std::int32_t>(payload[1]) << 16) |
                   (static_cast<std::int32_t>(payload[2]) << 8) | static_cast<std::int32_t>(payload[3]);
        payload += 4;

        players.push_back(info);
    }

    return players;
}

std::vector<std::uint8_t> buildRoomSetReadyPacket(std::uint32_t roomId, bool ready, std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::RoomSetReady);
    hdr.sequenceId  = sequence;

    std::uint16_t payloadSize = sizeof(std::uint32_t) + sizeof(std::uint8_t);
    hdr.payloadSize           = payloadSize;
    hdr.originalSize          = payloadSize;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    packet.push_back(ready ? 1 : 0);

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}
