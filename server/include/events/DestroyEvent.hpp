#pragma once

#include "ecs/Registry.hpp"

struct DestroyEvent
{
    EntityId id = 0;
};
