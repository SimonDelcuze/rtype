#pragma once

#include "components/Components.hpp"
#include "systems/MonsterSpawnSystem.hpp"

#include <utility>
#include <vector>

std::pair<std::vector<MovementComponent>, std::vector<SpawnEvent>> buildSpawnSetupForLevel(int levelId = 1);
