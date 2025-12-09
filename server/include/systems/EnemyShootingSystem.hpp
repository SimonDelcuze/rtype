#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

class EnemyShootingSystem
{
  public:
    EnemyShootingSystem() = default;
    void update(Registry& registry, float deltaTime);
};
