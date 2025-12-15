#pragma once

#include "components/Components.hpp"
#include "systems/MonsterSpawnSystem.hpp"
#include "systems/ObstacleSpawnSystem.hpp"

#include <vector>

struct LevelScript
{
    std::vector<MovementComponent> patterns;
    std::vector<SpawnEvent> spawns;
    std::vector<ObstacleSpawn> obstacles;
};
