#pragma once

#include "ecs/Registry.hpp"

#include <cstdint>

struct DamageEvent
{
    EntityId attacker      = 0;
    EntityId target        = 0;
    std::int32_t amount    = 0;
    std::int32_t remaining = 0;
};
