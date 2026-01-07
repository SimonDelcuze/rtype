#pragma once

#include "components/Components.hpp"
#include "levels/LevelScript.hpp"
#include "systems/MonsterSpawnSystem.hpp"

#include <utility>
#include <vector>

LevelScript buildSpawnSetupForLevel(int levelId = 1);
