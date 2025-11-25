#pragma once

#include <cstdint>

struct MissileComponent
{
    std::int32_t damage = 1;
    float lifetime      = 1.0F;
    bool fromPlayer     = true;
};
