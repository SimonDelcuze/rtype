#pragma once

#include "ecs/Registry.hpp"

class ISystem
{
  public:
    virtual ~ISystem() = default;

    virtual void initialize() {}
    virtual void update(Registry& registry, float deltaTime) = 0;
    virtual void cleanup() {}
};
