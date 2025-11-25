#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <cstddef>
#include <random>
#include <vector>

struct MonsterSpawnConfig
{
    float spawnInterval = 1.0F;
    float spawnX        = 0.0F;
    float yMin          = 0.0F;
    float yMax          = 0.0F;
};

class MonsterSpawnSystem
{
  public:
    MonsterSpawnSystem(MonsterSpawnConfig config, std::vector<MovementComponent> patterns, std::uint32_t seed = 1337);
    void update(Registry& registry, float deltaTime);

  private:
    MonsterSpawnConfig config_;
    std::vector<MovementComponent> patterns_;
    float accumulator_ = 0.0F;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> yDist_;
    std::uniform_int_distribution<std::size_t> patternDist_;
};
