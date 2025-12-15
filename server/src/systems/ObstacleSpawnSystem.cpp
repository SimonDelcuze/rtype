#include "systems/ObstacleSpawnSystem.hpp"

#include <algorithm>

ObstacleSpawnSystem::ObstacleSpawnSystem(std::vector<ObstacleSpawn> obstacles, float playfieldHeight)
    : obstacles_(std::move(obstacles)), playfieldHeight_(playfieldHeight)
{
    std::sort(obstacles_.begin(), obstacles_.end(), [](const auto& a, const auto& b) { return a.time < b.time; });
}

void ObstacleSpawnSystem::reset()
{
    elapsed_   = 0.0F;
    nextIndex_ = 0;
}

void ObstacleSpawnSystem::update(Registry& registry, float deltaTime)
{
    if (obstacles_.empty()) {
        return;
    }
    elapsed_ += deltaTime;
    while (nextIndex_ < obstacles_.size() && elapsed_ >= obstacles_[nextIndex_].time) {
        const auto& spawn = obstacles_[nextIndex_];
        EntityId e        = registry.createEntity();
        auto& t           = registry.emplace<TransformComponent>(e);
        t.x               = spawn.x;
        t.y               = resolveY(spawn);
        t.scaleX          = spawn.scaleX;
        t.scaleY          = spawn.scaleY;
        registry.emplace<TagComponent>(e, TagComponent::create(EntityTag::Obstacle));
        registry.emplace<HealthComponent>(e, HealthComponent::create(spawn.health));
        registry.emplace<VelocityComponent>(e, VelocityComponent::create(spawn.speedX, spawn.speedY));
        registry.emplace<HitboxComponent>(e, spawn.hitbox);
        registry.emplace<ColliderComponent>(e, spawn.collider);
        registry.emplace<RenderTypeComponent>(e, RenderTypeComponent::create(spawn.typeId));
        ++nextIndex_;
    }
}

float ObstacleSpawnSystem::resolveY(const ObstacleSpawn& spawn) const
{
    const float scaleY       = spawn.scaleY;
    const float scaledHeight = spawn.hitbox.height * scaleY;
    const float scaledOffset = spawn.hitbox.offsetY * scaleY;
    switch (spawn.anchor) {
        case ObstacleAnchor::Top:
            return spawn.margin - scaledOffset;
        case ObstacleAnchor::Bottom:
            return playfieldHeight_ - scaledHeight - scaledOffset - spawn.margin;
        case ObstacleAnchor::Absolute:
        default:
            return spawn.y;
    }
}
