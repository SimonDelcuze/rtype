#include "simulation/GameWorld.hpp"

#include "server/EntityTypeResolver.hpp"

GameWorld::GameWorld()
    : playerInputSys_(250.0F, 400.0F, 2.0F, 10), movementSys_(), monsterMovementSys_(), enemyShootingSys_(),
      collisionSys_(), damageSys_(eventBus_), scoreSys_(eventBus_, registry_), destructionSys_(eventBus_),
      boundarySys_(), introCinematic_()
{}

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

    trackEntityLifecycle();
}

std::vector<GameEvent> GameWorld::consumeEvents()
{
    std::vector<GameEvent> events = std::move(pendingEvents_);
    pendingEvents_.clear();
    return events;
}

void GameWorld::emitSpawn(EntityId id, std::uint8_t type, float x, float y)
{
    EntitySpawnedEvent evt{};
    evt.entityId   = id;
    evt.entityType = type;
    evt.posX       = x;
    evt.posY       = y;
    pendingEvents_.push_back(evt);
}

void GameWorld::emitDestroy(EntityId id)
{
    EntityDestroyedEvent evt{};
    evt.entityId = id;
    pendingEvents_.push_back(evt);
}

void GameWorld::trackEntityLifecycle()
{
    std::unordered_set<EntityId> current;
    for (EntityId id : registry_.view<TransformComponent>()) {
        if (registry_.isAlive(id)) {
            current.insert(id);
        }
    }

    for (EntityId id : current) {
        if (!knownEntities_.contains(id)) {
            std::uint8_t type = resolveEntityType(registry_, id);
            auto& t           = registry_.get<TransformComponent>(id);
            emitSpawn(id, type, t.x, t.y);
        }
    }

    for (EntityId oldId : knownEntities_) {
        if (!current.contains(oldId)) {
            emitDestroy(oldId);
        }
    }

    knownEntities_ = current;
}
