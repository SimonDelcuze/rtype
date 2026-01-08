#include "levels/SpawnConfig.hpp"

#include "levels/LevelFactory.hpp"

LevelScript buildSpawnSetupForLevel(int levelId)
{
    auto level = makeLevel(levelId);
    return level->buildScript();
}
