#include "server/SpawnConfig.hpp"

#include "levels/LevelFactory.hpp"

std::pair<std::vector<MovementComponent>, std::vector<SpawnEvent>> buildSpawnSetupForLevel(int levelId)
{
    auto level = makeLevel(levelId);
    auto script = level->buildScript();
    return {std::move(script.patterns), std::move(script.spawns)};
}
