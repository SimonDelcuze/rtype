#pragma once

#include "ecs/Registry.hpp"
#include "events/EventBus.hpp"
#include "levels/IntroCinematic.hpp"
#include "levels/LevelData.hpp"
#include "levels/LevelDirector.hpp"
#include "levels/LevelSpawnSystem.hpp"
#include "simulation/GameEvent.hpp"
#include "simulation/PlayerCommand.hpp"
#include "systems/BoundarySystem.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/DestructionSystem.hpp"
#include "systems/EnemyShootingSystem.hpp"
#include "systems/MonsterMovementSystem.hpp"
#include "systems/MovementSystem.hpp"
#include "systems/PlayerInputSystem.hpp"
#include "systems/ScoreSystem.hpp"

#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

class GameWorld
{
  public:
    GameWorld();

    void tick(float deltaTime, const std::vector<PlayerCommand>& commands,
              const std::map<std::uint32_t, EntityId>& playerEntities);

    std::vector<GameEvent> consumeEvents();

    const Registry& getRegistry() const
    {
        return registry_;
    }

    Registry& getRegistry()
    {
        return registry_;
    }

    CollisionSystem& getCollisionSystem()
    {
        return collisionSys_;
    }

    bool isLevelLoaded() const
    {
        return levelLoaded_;
    }

    void setLevelLoaded(bool loaded)
    {
        levelLoaded_ = loaded;
    }

    LevelDirector* getLevelDirector()
    {
        return levelDirector_.get();
    }

    void setLevelDirector(std::unique_ptr<LevelDirector> director)
    {
        levelDirector_ = std::move(director);
    }

    LevelSpawnSystem* getLevelSpawnSystem()
    {
        return levelSpawnSys_.get();
    }

    void setLevelSpawnSystem(std::unique_ptr<LevelSpawnSystem> system)
    {
        levelSpawnSys_ = std::move(system);
    }

    IntroCinematic& getIntroCinematic()
    {
        return introCinematic_;
    }

    void trackEntityLifecycle();

  private:
    void emitSpawn(EntityId id, std::uint8_t type, float x, float y);
    void emitDestroy(EntityId id);

    Registry registry_;
    EventBus eventBus_;
    std::vector<GameEvent> pendingEvents_;
    std::unordered_set<EntityId> knownEntities_;

    PlayerInputSystem playerInputSys_;
    MovementSystem movementSys_;
    MonsterMovementSystem monsterMovementSys_;
    EnemyShootingSystem enemyShootingSys_;
    CollisionSystem collisionSys_;
    DamageSystem damageSys_;
    ScoreSystem scoreSys_;
    DestructionSystem destructionSys_;
    BoundarySystem boundarySys_;

    IntroCinematic introCinematic_;
    std::unique_ptr<LevelDirector> levelDirector_;
    std::unique_ptr<LevelSpawnSystem> levelSpawnSys_;
    bool levelLoaded_{false};
};
