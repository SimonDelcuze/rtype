#pragma once

#include "ecs/Registry.hpp"

class ShieldSystem
{
  public:
    ShieldSystem() = default;
    void update(Registry& registry, float deltaTime);

  private:
    static constexpr float kHorizontalOffset = 40.0F;
};
