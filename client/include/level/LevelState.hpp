#pragma once

#include <cstdint>

struct LevelState
{
    std::uint16_t levelId = 0;
    std::uint32_t seed    = 0;
    bool active           = false;
};
