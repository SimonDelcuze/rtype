#pragma once

#include "ecs/Registry.hpp"

class AllySystem
{
  public:
    AllySystem() = default;
    void update(Registry& registry, float deltaTime);

  private:
    static constexpr float kVerticalOffset       = 30.0F;
    static constexpr float kMissileSpeed         = 400.0F;
    static constexpr float kMissileLifetime      = 2.0F;
    static constexpr std::int32_t kMissileDamage = 10;
};
