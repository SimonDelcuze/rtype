#include "systems/ReplicationSystem.hpp"

#include "components/HealthComponent.hpp"
#include "components/InterpolationComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"
#include "ecs/Registry.hpp"

#include <iostream>

ReplicationSystem::ReplicationSystem(ThreadSafeQueue<SnapshotParseResult>& snapshots) : snapshots_(&snapshots) {}

void ReplicationSystem::initialize() {}

void ReplicationSystem::update(Registry& registry, float)
{
    SnapshotParseResult snapshot;
    while (snapshots_->tryPop(snapshot)) {
        for (const auto& entity : snapshot.entities) {
            auto localId = ensureEntity(registry, entity.entityId);
            applyEntity(registry, localId, entity);
            if (!registry.isAlive(localId)) {
                continue;
            }
            applyInterpolation(registry, localId, entity, snapshot.header.tickId);
        }
    }
}

void ReplicationSystem::cleanup()
{
    remoteToLocal_.clear();
}

EntityId ReplicationSystem::ensureEntity(Registry& registry, std::uint32_t remoteId)
{
    auto it = remoteToLocal_.find(remoteId);
    if (it != remoteToLocal_.end() && registry.isAlive(it->second)) {
        return it->second;
    }
    EntityId id              = registry.createEntity();
    remoteToLocal_[remoteId] = id;
    return id;
}

void ReplicationSystem::applyEntity(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    applyTransform(registry, id, entity);
    applyVelocity(registry, id, entity);
    applyHealth(registry, id, entity);
    applyStatus(registry, id, entity);
    applyDead(registry, id, entity);
}

void ReplicationSystem::applyTransform(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    if (!entity.posX.has_value() && !entity.posY.has_value()) {
        return;
    }
    auto* comp = registry.has<TransformComponent>(id) ? &registry.get<TransformComponent>(id) : nullptr;
    if (comp == nullptr) {
        TransformComponent t{};
        if (entity.posX.has_value())
            t.x = *entity.posX;
        if (entity.posY.has_value())
            t.y = *entity.posY;
        registry.emplace<TransformComponent>(id, t);
        return;
    }
    if (entity.posX.has_value())
        comp->x = *entity.posX;
    if (entity.posY.has_value())
        comp->y = *entity.posY;
}

void ReplicationSystem::applyVelocity(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    if (!entity.velX.has_value() && !entity.velY.has_value()) {
        return;
    }
    auto* comp = registry.has<VelocityComponent>(id) ? &registry.get<VelocityComponent>(id) : nullptr;
    if (comp == nullptr) {
        VelocityComponent v{};
        if (entity.velX.has_value())
            v.vx = *entity.velX;
        if (entity.velY.has_value())
            v.vy = *entity.velY;
        registry.emplace<VelocityComponent>(id, v);
        return;
    }
    if (entity.velX.has_value())
        comp->vx = *entity.velX;
    if (entity.velY.has_value())
        comp->vy = *entity.velY;
}

void ReplicationSystem::applyHealth(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    if (!entity.health.has_value()) {
        return;
    }
    auto* comp = registry.has<HealthComponent>(id) ? &registry.get<HealthComponent>(id) : nullptr;
    if (comp == nullptr) {
        registry.emplace<HealthComponent>(id, HealthComponent::create(*entity.health));
        return;
    }
    comp->current = *entity.health;
    if (*entity.health > comp->max) {
        comp->max = *entity.health;
    }
}

void ReplicationSystem::applyStatus(Registry&, EntityId, const SnapshotEntity&) {}

void ReplicationSystem::applyDead(Registry& registry, EntityId id, const SnapshotEntity& entity)
{
    if (!entity.dead.has_value()) {
        return;
    }
    if (*entity.dead && registry.isAlive(id)) {
        registry.destroyEntity(id);
    }
}

void ReplicationSystem::applyInterpolation(Registry& registry, EntityId id, const SnapshotEntity& entity, std::uint32_t)
{
    if (!entity.posX.has_value() && !entity.posY.has_value()) {
        return;
    }
    auto* interp = registry.has<InterpolationComponent>(id) ? &registry.get<InterpolationComponent>(id) : nullptr;
    if (interp == nullptr) {
        InterpolationComponent ic{};
        ic.previousX = entity.posX.value_or(0.0F);
        ic.previousY = entity.posY.value_or(0.0F);
        ic.targetX   = entity.posX.value_or(ic.previousX);
        ic.targetY   = entity.posY.value_or(ic.previousY);
        ic.velocityX = entity.velX.value_or(0.0F);
        ic.velocityY = entity.velY.value_or(0.0F);
        registry.emplace<InterpolationComponent>(id, ic);
        return;
    }
    float prevX         = interp->targetX;
    float prevY         = interp->targetY;
    interp->previousX   = prevX;
    interp->previousY   = prevY;
    interp->targetX     = entity.posX.value_or(prevX);
    interp->targetY     = entity.posY.value_or(prevY);
    interp->elapsedTime = 0.0F;
    if (entity.velX.has_value())
        interp->velocityX = *entity.velX;
    if (entity.velY.has_value())
        interp->velocityY = *entity.velY;
}
