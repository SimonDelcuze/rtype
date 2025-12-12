#include "systems/MonsterSpawnSystem.hpp"

#include <algorithm>

MonsterSpawnSystem::MonsterSpawnSystem(std::vector<MovementComponent> patterns, std::vector<SpawnEvent> script)
    : patterns_(std::move(patterns)), script_(std::move(script))
{}

void MonsterSpawnSystem::reset()
{
    elapsed_   = 0.0F;
    nextIndex_ = 0;
}

void MonsterSpawnSystem::update(Registry& registry, float deltaTime)
{
    if (patterns_.empty() || script_.empty())
        return;

    elapsed_ += deltaTime;

    while (nextIndex_ < script_.size() && elapsed_ >= script_[nextIndex_].time) {
        const auto& ev = script_[nextIndex_];
        EntityId e     = registry.createEntity();

        auto& t  = registry.emplace<TransformComponent>(e);
        t.x      = ev.x;
        t.y      = ev.y;
        t.scaleX = ev.scaleX;
        t.scaleY = ev.scaleY;

        std::size_t patternIdx = std::min(ev.pattern, patterns_.size() - 1);
        registry.emplace<MovementComponent>(e, patterns_[patternIdx]);
        registry.emplace<VelocityComponent>(e);
        registry.emplace<TagComponent>(e, TagComponent::create(EntityTag::Enemy));
        registry.emplace<HealthComponent>(e, HealthComponent::create(ev.health));
        registry.emplace<HitboxComponent>(e, ev.hitbox);
        if (ev.shootingEnabled) {
            registry.emplace<EnemyShootingComponent>(e, ev.shooting);
        }

        ++nextIndex_;
    }
}
