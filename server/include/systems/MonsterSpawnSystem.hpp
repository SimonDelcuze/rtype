#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <cstddef>
#include <random>
#include <vector>

struct SpawnEvent
{
    float time           = 0.0F;
    float x              = 0.0F;
    float y              = 0.0F;
    std::size_t pattern  = 0;
    std::int32_t health  = 50;
    float scaleX         = 1.0F;
    float scaleY         = 1.0F;
    bool shootingEnabled = true;
    HitboxComponent hitbox;
    EnemyShootingComponent shooting;
};

class MonsterSpawnSystem
{
  public:
    MonsterSpawnSystem(std::vector<MovementComponent> patterns, std::vector<SpawnEvent> script);
    void update(Registry& registry, float deltaTime);
    void reset();

  private:
    std::vector<MovementComponent> patterns_;
    std::vector<SpawnEvent> script_;
    float elapsed_         = 0.0F;
    std::size_t nextIndex_ = 0;
};
