#pragma once

#include "components/Components.hpp"
#include "config/WorldConfig.hpp"
#include "ecs/Registry.hpp"

#include <random>
#include <string>
#include <vector>

struct ObstacleVariant
{
    std::uint16_t typeId    = 0;
    std::uint16_t topTypeId = 0;
    std::string spriteId;
    std::string topSpriteId;
    float spriteWidth  = 0.0F;
    float spriteHeight = 0.0F;
    float hitboxWidth     = 0.0F;
    float hitboxHeight    = 0.0F;
    float bottomOffsetY   = 0.0F;
    float topOffsetY      = 0.0F;
};

struct ObstacleSpawnConfig
{
    float spawnInterval = 5.5F;
    float spawnX        = WorldConfig::Width + 200.0F;
    float speed         = WorldConfig::BackgroundSpeedX * 1.8F;
    float intervalJitter =
        0.25F; // multiplier variation applied to spawnInterval (e.g. 0.25 -> [0.75, 1.25] * interval)
    float cleanupMargin = 450.0F;
    float topSpawnChance = 0.5F;
};

class ObstacleSpawnSystem
{
  public:
    ObstacleSpawnSystem(ObstacleSpawnConfig config, std::vector<ObstacleVariant> variants);

    void setSeed(std::uint32_t seed);
    void update(Registry& registry, float deltaTime);

    float cleanupLeftBound() const;
    float cleanupRightBound() const;

  private:
    void scheduleNextSpawn();
    void spawnOne(Registry& registry, const ObstacleVariant& variant, bool flipVertical);

    ObstacleSpawnConfig config_;
    std::vector<ObstacleVariant> variants_;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> jitterDist_;
    std::bernoulli_distribution topDist_;
    std::uniform_int_distribution<std::size_t> variantDist_;
    float accumulator_    = 0.0F;
    float timeToNextSpawn_ = 0.0F;
};
