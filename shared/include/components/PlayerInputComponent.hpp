#pragma once

#include <cstdint>

struct PlayerInputComponent
{
    float x                  = 0.0F;
    float y                  = 0.0F;
    float angle              = 0.0F;
    std::uint16_t sequenceId = 0;
};
