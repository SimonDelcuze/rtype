#include "systems/ReplicationSystem.hpp"

#include "components/AnimationComponent.hpp"
#include "components/HealthComponent.hpp"
#include "components/InterpolationComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"
#include "ecs/Registry.hpp"
#include "animation/AnimationRegistry.hpp"

#include <iostream>

ReplicationSystem::ReplicationSystem(ThreadSafeQueue<SnapshotParseResult>& snapshots, const EntityTypeRegistry& types)
    : snapshots_(&snapshots), types_(&types)
{}

void ReplicationSystem::initialize() {}

void ReplicationSystem::update(Registry& registry, float)
{
    SnapshotParseResult snapshot;
    std::size_t processed = 0;
    while (snapshots_->tryPop(snapshot)) {
        for (const auto& entity : snapshot.entities) {
            auto localId = ensureEntity(registry, entity);
            if (!localId.has_value()) {
                continue;
            }
            applyEntity(registry, *localId, entity);
            if (!registry.isAlive(*localId)) {
                continue;
            }
            applyInterpolation(registry, *localId, entity, snapshot.header.tickId);
        }
        processed += snapshot.entities.size();
    }
    if (processed > 0) {
        std::cout << "[client] Replication applied " << processed << " entities\n";
    }
}

void ReplicationSystem::cleanup()
{
    remoteToLocal_.clear();
}

std::optional<EntityId> ReplicationSystem::ensureEntity(Registry& registry, const SnapshotEntity& entity)
{
    auto remoteId = entity.entityId;
    auto it       = remoteToLocal_.find(remoteId);
    if (it != remoteToLocal_.end() && registry.isAlive(it->second)) {
        return it->second;
    }

    if (!entity.entityType.has_value()) {
        std::cerr << "[ReplicationSystem] Missing entityType for remoteId " << remoteId << '\n';
        return std::nullopt;
    }
    if (!types_->has(*entity.entityType)) {
        std::cerr << "[ReplicationSystem] Unknown entityType " << static_cast<int>(*entity.entityType)
                  << " for remoteId " << remoteId << '\n';
        return std::nullopt;
    }

    EntityId id              = registry.createEntity();
    remoteToLocal_[remoteId] = id;
    applyArchetype(registry, id, *entity.entityType);
    return std::optional<EntityId>(id);
}

void ReplicationSystem::applyArchetype(Registry& registry, EntityId id, std::uint16_t typeId)
{
    const auto* data = types_->get(typeId);
    if (data == nullptr) {
        return;
    }
    if (data->texture != nullptr) {
        SpriteComponent sprite(*data->texture);
        if (data->animation != nullptr) {
            const auto* clip = reinterpret_cast<const AnimationClip*>(data->animation);
            sprite.customFrames.clear();
            sprite.customFrames.reserve(clip->frames.size());
            for (const auto& f : clip->frames) {
                sprite.customFrames.emplace_back(sf::Vector2i{f.x, f.y}, sf::Vector2i{f.width, f.height});
            }
            if (!sprite.customFrames.empty()) {
                sprite.setFrame(0);
            }
        }
        if (data->frameWidth > 0 && data->frameHeight > 0) {
            sprite.setFrameSize(data->frameWidth, data->frameHeight,
                                data->columns == 0 ? 1 : data->columns);
        }
        registry.emplace<SpriteComponent>(id, sprite);
    } else {
        registry.emplace<SpriteComponent>(id, SpriteComponent{});
    }
    registry.emplace<LayerComponent>(id, LayerComponent::create(static_cast<int>(data->layer)));
    if (data->frameCount > 1) {
        auto anim = AnimationComponent::create(data->frameCount, data->frameDuration);
        if (data->animation != nullptr) {
            const auto* clip = reinterpret_cast<const AnimationClip*>(data->animation);
            anim.frameIndices.clear();
            for (std::uint32_t i = 0; i < clip->frames.size(); ++i) {
                anim.frameIndices.push_back(i);
            }
            anim.frameTime = clip->frameTime;
            anim.loop      = clip->loop;
        }
        registry.emplace<AnimationComponent>(id, anim);
    }
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
        for (auto it = remoteToLocal_.begin(); it != remoteToLocal_.end(); ++it) {
            if (it->second == id) {
                remoteToLocal_.erase(it);
                break;
            }
        }
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
