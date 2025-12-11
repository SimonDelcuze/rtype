#include "systems/ObstacleSpawnSystem.hpp"

#include "config/EntityTypeIds.hpp"

#include <algorithm>

namespace
{
    float clampJitter(float jitter)
    {
        return std::clamp(jitter, 0.0F, 0.95F);
    }
} // namespace

ObstacleSpawnSystem::ObstacleSpawnSystem(ObstacleSpawnConfig config, std::vector<ObstacleVariant> variants)
    : config_(config), variants_(std::move(variants)), jitterDist_(1.0F - clampJitter(config.intervalJitter),
                                                                   1.0F + clampJitter(config.intervalJitter)),
      topDist_(config.topSpawnChance),
      variantDist_(0, variants_.empty() ? 0 : variants_.size() - 1)
{
    std::random_device rd;
    rng_.seed(rd());
    scheduleNextSpawn();
}

void ObstacleSpawnSystem::setSeed(std::uint32_t seed)
{
    rng_.seed(seed);
    accumulator_ = 0.0F;
    scheduleNextSpawn();
}

void ObstacleSpawnSystem::update(Registry& registry, float deltaTime)
{
    if (variants_.empty() || timeToNextSpawn_ <= 0.0F) {
        return;
    }
    accumulator_ += deltaTime;
    while (accumulator_ >= timeToNextSpawn_) {
        accumulator_ -= timeToNextSpawn_;
        const auto& variant = variants_[variantDist_(rng_)];
        const bool top      = topDist_(rng_);
        spawnOne(registry, variant, top);
        scheduleNextSpawn();
    }
}

float ObstacleSpawnSystem::cleanupLeftBound() const
{
    return -config_.cleanupMargin;
}

float ObstacleSpawnSystem::cleanupRightBound() const
{
    return config_.spawnX + config_.cleanupMargin;
}

void ObstacleSpawnSystem::scheduleNextSpawn()
{
    if (variants_.empty())
        return;
    float multiplier  = jitterDist_(rng_);
    timeToNextSpawn_  = std::max(0.1F, config_.spawnInterval * multiplier);
}

void ObstacleSpawnSystem::spawnOne(Registry& registry, const ObstacleVariant& variant, bool topSpawn)
{
    EntityId e      = registry.createEntity();
    auto& transform = registry.emplace<TransformComponent>(e);
    transform.x     = config_.spawnX;

    transform.y = topSpawn ? 0.0F : std::max(0.0F, WorldConfig::Height - variant.spriteHeight);

    registry.emplace<VelocityComponent>(e, VelocityComponent::create(config_.speed, 0.0F));
    registry.emplace<TagComponent>(e, TagComponent::create(EntityTag::Obstacle));
    registry.emplace<TypeComponent>(
        e, TypeComponent::create(topSpawn && variant.topTypeId != 0 ? variant.topTypeId : variant.typeId));

    const float marginX = std::max(0.0F, (variant.spriteWidth - variant.hitboxWidth) / 2.0F);
    const float marginY = std::max(0.0F, (variant.spriteHeight - variant.hitboxHeight) / 2.0F);
    const float offsetX = marginX + variant.hitboxWidth / 2.0F;
    const float offsetY = marginY + variant.hitboxHeight / 2.0F;

    registry.emplace<HitboxComponent>(
        e, HitboxComponent::create(variant.hitboxWidth, variant.hitboxHeight, offsetX, offsetY, true));
}
