#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

enum class MessageType : std::uint8_t
{
    Invalid            = 0x00,
    ClientHello        = 0x01,
    ClientJoinRequest  = 0x02,
    ClientReady        = 0x03,
    ClientPing         = 0x04,
    Input              = 0x05,
    ClientAcknowledge  = 0x06,
    ClientDisconnect   = 0x07,
    ServerHello        = 0x10,
    ServerJoinAccept   = 0x11,
    ServerJoinDeny     = 0x12,
    ServerPong         = 0x13,
    Snapshot           = 0x14,
    SnapshotChunk      = 0x21,
    GameStart          = 0x15,
    GameEnd            = 0x16,
    ServerKick         = 0x17,
    ServerBan          = 0x18,
    ServerBroadcast    = 0x19,
    ServerDisconnect   = 0x1A,
    ServerAcknowledge  = 0x1B,
    PlayerDisconnected = 0x1C,
    EntitySpawn        = 0x1D,
    EntityDestroyed    = 0x1E,
    AllReady           = 0x1F,
    CountdownTick      = 0x20,
    LevelInit          = 0x30,
    LevelTransition    = 0x31,
    Handshake          = ClientHello,
    Ack                = ClientAcknowledge
};

enum class PacketType : std::uint8_t
{
    ClientToServer = 0x01,
    ServerToClient = 0x02
};

struct PacketHeader
{
    std::uint8_t version      = 1;
    std::uint8_t packetType   = static_cast<std::uint8_t>(PacketType::ClientToServer);
    std::uint8_t messageType  = static_cast<std::uint8_t>(MessageType::Invalid);
    std::uint16_t sequenceId  = 0;
    std::uint32_t tickId      = 0;
    std::uint16_t payloadSize = 0;

    static constexpr std::array<std::uint8_t, 4> kMagic = {0xA3, 0x5F, 0xC8, 0x1D};
    static constexpr std::uint8_t kProtocolVersion      = 1;
    static constexpr std::size_t kSize                  = 15;
    static constexpr std::size_t kCrcSize               = 4;

    [[nodiscard]] inline std::array<std::uint8_t, kSize> encode() const noexcept
    {
        std::array<std::uint8_t, kSize> out{};
        out[0]  = kMagic[0];
        out[1]  = kMagic[1];
        out[2]  = kMagic[2];
        out[3]  = kMagic[3];
        out[4]  = version;
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
        return out;
    }

    [[nodiscard]] static inline std::optional<PacketHeader> decode(const std::uint8_t* data, std::size_t len) noexcept
    {
        if (data == nullptr || len < kSize)
            return std::nullopt;
        if (data[0] != kMagic[0] || data[1] != kMagic[1] || data[2] != kMagic[2] || data[3] != kMagic[3])
            return std::nullopt;
        if (data[4] != kProtocolVersion)
            return std::nullopt;
        PacketHeader h{};
        h.version     = data[4];
        h.packetType  = data[5];
        h.messageType = data[6];
        h.sequenceId  = static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[7]) << 8) |
                                                   static_cast<std::uint16_t>(data[8]));
        h.tickId      = (static_cast<std::uint32_t>(data[9]) << 24) | (static_cast<std::uint32_t>(data[10]) << 16) |
                   (static_cast<std::uint32_t>(data[11]) << 8) | static_cast<std::uint32_t>(data[12]);
        h.payloadSize = static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[13]) << 8) |
                                                   static_cast<std::uint16_t>(data[14]));
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

static_assert(PacketHeader::kSize == 15, "PacketHeader wire size must remain 15 bytes");
