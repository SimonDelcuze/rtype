#include "network/InputParser.hpp"

#include <cstdint>

namespace
{
    constexpr std::uint16_t allowedFlags =
        static_cast<std::uint16_t>(InputFlag::MoveUp) | static_cast<std::uint16_t>(InputFlag::MoveDown) |
        static_cast<std::uint16_t>(InputFlag::MoveLeft) | static_cast<std::uint16_t>(InputFlag::MoveRight) |
        static_cast<std::uint16_t>(InputFlag::Fire);
}

std::optional<ServerInput> parseInputPacket(const std::uint8_t* data, std::size_t len)
{
    auto decoded = InputPacket::decode(data, len);
    if (!decoded)
        return std::nullopt;
    if ((decoded->flags & static_cast<std::uint16_t>(~allowedFlags)) != 0)
        return std::nullopt;
    ServerInput out{};
    out.playerId   = decoded->playerId;
    out.flags      = decoded->flags;
    out.x          = decoded->x;
    out.y          = decoded->y;
    out.angle      = decoded->angle;
    out.sequenceId = decoded->header.sequenceId;
    out.tickId     = decoded->header.tickId;
    return out;
}
