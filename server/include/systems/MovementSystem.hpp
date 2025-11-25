#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

class MovementSystem
{
  public:
    void update(Registry& registry, float deltaTime) const;
};
