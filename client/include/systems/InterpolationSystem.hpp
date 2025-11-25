#pragma once

#include "ecs/Registry.hpp"

class InterpolationSystem
{
  public:
    void update(Registry& registry, float deltaTime);

  private:
    float lerp(float start, float end, float t) const;
    float clamp(float value, float min, float max) const;
};
