#pragma once

#include "components/Components.hpp"
#include "systems/MonsterSpawnSystem.hpp"

#include <vector>

struct LevelScript
{
    std::vector<MovementComponent> patterns;
    std::vector<SpawnEvent> spawns;
};

