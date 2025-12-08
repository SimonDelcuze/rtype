#include "systems/MonsterSpawnSystem.hpp"

MonsterSpawnSystem::MonsterSpawnSystem(MonsterSpawnConfig config, std::vector<MovementComponent> patterns,
                                       std::uint32_t seed)
    : config_(config), patterns_(std::move(patterns)), rng_(seed), yDist_(config.yMin, config.yMax),
      patternDist_(0, patterns_.empty() ? 0 : patterns_.size() - 1)
{}

void MonsterSpawnSystem::update(Registry& registry, float deltaTime)
{
    if (patterns_.empty())
        return;
    accumulator_ += deltaTime;
    while (accumulator_ >= config_.spawnInterval) {
        accumulator_ -= config_.spawnInterval;
        float y                      = yDist_(rng_);
        const MovementComponent& pat = patterns_[patternDist_(rng_)];
        EntityId e                   = registry.createEntity();
        auto& t                      = registry.emplace<TransformComponent>(e);
        t.x                          = config_.spawnX;
        t.y                          = y;
        registry.emplace<MovementComponent>(e, pat);
        registry.emplace<VelocityComponent>(e);
        registry.emplace<TagComponent>(e, TagComponent::create(EntityTag::Enemy));
        registry.emplace<HealthComponent>(e, HealthComponent::create(50));
        registry.emplace<HitboxComponent>(e, HitboxComponent::create(50.0F, 50.0F, 0.0F, 0.0F, true));
    }
}
