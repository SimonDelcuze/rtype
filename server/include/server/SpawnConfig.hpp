#pragma once

#include "components/Components.hpp"
#include "server/LevelScript.hpp"
#include "systems/MonsterSpawnSystem.hpp"

#include <utility>
#include <vector>

LevelScript buildSpawnSetupForLevel(int levelId = 1);
