#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

enum class MessageType : std::uint8_t
{
    Invalid   = 0,
    Snapshot  = 1,
    Input     = 2,
    Handshake = 3,
    Ack       = 4
};

struct PacketHeader
{
    std::uint8_t messageType = static_cast<std::uint8_t>(MessageType::Invalid);
    std::uint16_t sequenceId = 0;
    std::uint32_t tickId = 0;

    static constexpr std::size_t kSize = 7;

    [[nodiscard]] inline std::array<std::uint8_t, kSize> encode() const noexcept
    {
        std::array<std::uint8_t, kSize> out{};
        out[0] = messageType;
        out[1] = static_cast<std::uint8_t>((sequenceId >> 8) & 0xFF);
        out[2] = static_cast<std::uint8_t>((sequenceId) & 0xFF);
        out[3] = static_cast<std::uint8_t>((tickId >> 24) & 0xFF);
        out[4] = static_cast<std::uint8_t>((tickId >> 16) & 0xFF);
        out[5] = static_cast<std::uint8_t>((tickId >> 8) & 0xFF);
        out[6] = static_cast<std::uint8_t>((tickId) & 0xFF);
        return out;
    }

    [[nodiscard]] static inline std::optional<PacketHeader> decode(const std::uint8_t* data, std::size_t len) noexcept
    {
        if (data == nullptr || len < kSize)
            return std::nullopt;
        PacketHeader h{};
        h.messageType = data[0];
        h.sequenceId  = static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[1]) << 8) |
                                                  static_cast<std::uint16_t>(data[2]));
        h.tickId      = (static_cast<std::uint32_t>(data[3]) << 24) |
                   (static_cast<std::uint32_t>(data[4]) << 16) |
                   (static_cast<std::uint32_t>(data[5]) << 8) |
                   static_cast<std::uint32_t>(data[6]);
        return h;
    }
};

static_assert(PacketHeader::kSize == 7, "PacketHeader wire size must remain 7 bytes");
