#pragma once

#include <cstdint>

struct PlayerCommand
{
    std::uint32_t playerId;
    std::uint16_t inputFlags;
    float x;
    float y;
    float angle;
    std::uint16_t sequenceId;
    std::uint32_t tickId;
};
