#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

enum class MessageType : std::uint8_t
{
    Invalid                    = 0x00,
    ClientHello                = 0x01,
    ClientJoinRequest          = 0x02,
    ClientReady                = 0x03,
    ClientPing                 = 0x04,
    Input                      = 0x05,
    ClientAcknowledge          = 0x06,
    ClientDisconnect           = 0x07,
    ServerHello                = 0x10,
    ServerJoinAccept           = 0x11,
    ServerJoinDeny             = 0x12,
    ServerPong                 = 0x13,
    Snapshot                   = 0x14,
    SnapshotChunk              = 0x21,
    GameStart                  = 0x15,
    GameEnd                    = 0x16,
    ServerKick                 = 0x17,
    ServerBan                  = 0x18,
    ServerBroadcast            = 0x19,
    ServerDisconnect           = 0x1A,
    ServerAcknowledge          = 0x1B,
    PlayerDisconnected         = 0x1C,
    EntitySpawn                = 0x1D,
    EntityDestroyed            = 0x1E,
    AllReady                   = 0x1F,
    CountdownTick              = 0x20,
    LevelInit                  = 0x30,
    LevelTransition            = 0x31,
    LevelEvent                 = 0x32,
    LobbyListRooms             = 0x40,
    LobbyRoomList              = 0x41,
    LobbyCreateRoom            = 0x42,
    LobbyRoomCreated           = 0x43,
    LobbyJoinRoom              = 0x44,
    LobbyJoinSuccess           = 0x45,
    LobbyJoinFailed            = 0x46,
    LobbyPasswordRequired      = 0x47,
    LobbyPasswordIncorrect     = 0x48,
    LobbyLeaveRoom             = 0x49,
    AuthLoginRequest           = 0x50,
    AuthLoginResponse          = 0x51,
    AuthRegisterRequest        = 0x52,
    AuthRegisterResponse       = 0x53,
    AuthChangePasswordRequest  = 0x54,
    AuthChangePasswordResponse = 0x55,
    AuthTokenRefreshRequest    = 0x56,
    AuthTokenRefreshResponse   = 0x57,
    AuthRequired               = 0x58,
    AuthGetStatsRequest        = 0x59,
    AuthGetStatsResponse       = 0x5A,
    StateChecksum              = 0x60,
    RollbackRequest            = 0x61,
    DesyncDetected             = 0x62,
    RoomKickPlayer             = 0x63,
    RoomBanPlayer              = 0x64,
    RoomPromoteAdmin           = 0x65,
    RoomDemoteAdmin            = 0x66,
    RoomTransferOwner          = 0x67,
    RoomPlayerKicked           = 0x68,
    RoomPlayerBanned           = 0x69,
    RoomRoleChanged            = 0x6A,
    RoomGetPlayers             = 0x6B,
    RoomPlayerList             = 0x6C,
    RoomForceStart             = 0x6D,
    RoomGameStarting           = 0x6E,
    RoomSetPlayerCount         = 0x6F,
    Chat                       = 0x70,
    Handshake                  = ClientHello,
    Ack                        = ClientAcknowledge
};

enum class PacketType : std::uint8_t
{
    ClientToServer = 0x01,
    ServerToClient = 0x02
};

enum class PlayerRole : std::uint8_t
{
    Owner     = 0,
    Admin     = 1,
    Player    = 2,
    Spectator = 3,
    Banned    = 4
};

struct PacketHeader
{
    std::uint8_t version       = kProtocolVersion;
    bool isCompressed          = false;
    std::uint8_t packetType    = static_cast<std::uint8_t>(PacketType::ClientToServer);
    std::uint8_t messageType   = static_cast<std::uint8_t>(MessageType::Invalid);
    std::uint16_t sequenceId   = 0;
    std::uint32_t tickId       = 0;
    std::uint16_t payloadSize  = 0;
    std::uint16_t originalSize = 0;

    static constexpr std::array<std::uint8_t, 4> kMagic = {0xA3, 0x5F, 0xC8, 0x1D};
    static constexpr std::uint8_t kProtocolVersion      = 1;
    static constexpr std::size_t kSize                  = 17;
    static constexpr std::size_t kCrcSize               = 4;

    [[nodiscard]] inline std::array<std::uint8_t, kSize> encode() const noexcept
    {
        std::array<std::uint8_t, kSize> out{};
        out[0]  = kMagic[0];
        out[1]  = kMagic[1];
        out[2]  = kMagic[2];
        out[3]  = kMagic[3];
        out[4]  = static_cast<std::uint8_t>(version | (isCompressed ? 0x80 : 0x00));
        out[5]  = packetType;
        out[6]  = messageType;
        out[7]  = static_cast<std::uint8_t>((sequenceId >> 8) & 0xFF);
        out[8]  = static_cast<std::uint8_t>(sequenceId & 0xFF);
        out[9]  = static_cast<std::uint8_t>((tickId >> 24) & 0xFF);
        out[10] = static_cast<std::uint8_t>((tickId >> 16) & 0xFF);
        out[11] = static_cast<std::uint8_t>((tickId >> 8) & 0xFF);
        out[12] = static_cast<std::uint8_t>(tickId & 0xFF);
        out[13] = static_cast<std::uint8_t>((payloadSize >> 8) & 0xFF);
        out[14] = static_cast<std::uint8_t>(payloadSize & 0xFF);
        out[15] = static_cast<std::uint8_t>((originalSize >> 8) & 0xFF);
        out[16] = static_cast<std::uint8_t>(originalSize & 0xFF);
        return out;
    }

    [[nodiscard]] static inline std::optional<PacketHeader> decode(const std::uint8_t* data, std::size_t len) noexcept
    {
        if (data == nullptr || len < kSize)
            return std::nullopt;
        if (data[0] != kMagic[0] || data[1] != kMagic[1] || data[2] != kMagic[2] || data[3] != kMagic[3])
            return std::nullopt;
        if ((data[4] & 0x7F) != kProtocolVersion)
            return std::nullopt;
        PacketHeader h{};
        h.version      = data[4] & 0x7F;
        h.isCompressed = (data[4] & 0x80) != 0;
        h.packetType   = data[5];
        h.messageType  = data[6];
        h.sequenceId   = static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[7]) << 8) |
                                                    static_cast<std::uint16_t>(data[8]));
        h.tickId       = (static_cast<std::uint32_t>(data[9]) << 24) | (static_cast<std::uint32_t>(data[10]) << 16) |
                   (static_cast<std::uint32_t>(data[11]) << 8) | static_cast<std::uint32_t>(data[12]);
        h.payloadSize  = static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[13]) << 8) |
                                                    static_cast<std::uint16_t>(data[14]));
        h.originalSize = static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[15]) << 8) |
                                                    static_cast<std::uint16_t>(data[16]));
        return h;
    }

    [[nodiscard]] static inline std::uint32_t crc32(const std::uint8_t* data, std::size_t len) noexcept
    {
        if (data == nullptr)
            return 0;
        std::uint32_t crc = 0xFFFFFFFFu;
        for (std::size_t i = 0; i < len; ++i) {
            crc ^= static_cast<std::uint32_t>(data[i]);
            for (int b = 0; b < 8; ++b) {
                const bool lsb = (crc & 1u) != 0u;
                crc >>= 1;
                if (lsb)
                    crc ^= 0xEDB88320u;
            }
        }
        return ~crc;
    }
};

static_assert(PacketHeader::kSize == 17, "PacketHeader wire size must remain 17 bytes");
