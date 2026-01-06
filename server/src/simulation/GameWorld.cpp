#include "simulation/GameWorld.hpp"

GameWorld::GameWorld()
    : playerInputSys_(250.0F, 400.0F, 2.0F, 10),
      movementSys_(),
      monsterMovementSys_(),
      enemyShootingSys_(),
      collisionSys_(),
      damageSys_(eventBus_),
      scoreSys_(eventBus_, registry_),
      destructionSys_(eventBus_),
      boundarySys_(),
      introCinematic_()
{
}

void GameWorld::tick(float deltaTime, const std::vector<PlayerCommand>& commands,
                     const std::map<std::uint32_t, EntityId>& playerEntities)
{
    introCinematic_.update(registry_, playerEntities, deltaTime);
    const bool introActive = introCinematic_.active();

    if (!introActive) {
        playerInputSys_.update(registry_, commands);
    }

    movementSys_.update(registry_, deltaTime);
    boundarySys_.update(registry_);
    monsterMovementSys_.update(registry_, deltaTime);

    if (levelLoaded_ && levelDirector_ && levelSpawnSys_) {
        float levelDelta = introActive ? 0.0F : deltaTime;
        levelDirector_->update(registry_, levelDelta);
        auto events = levelDirector_->consumeEvents();
        levelSpawnSys_->update(registry_, levelDelta, events);
    }

    enemyShootingSys_.update(registry_, deltaTime);

    auto collisions = collisionSys_.detect(registry_);
    damageSys_.apply(registry_, collisions);
}
