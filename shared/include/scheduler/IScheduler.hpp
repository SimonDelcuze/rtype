#pragma once

#include "systems/ISystem.hpp"

#include <memory>

class IScheduler
{
  public:
    virtual ~IScheduler() = default;

    virtual void addSystem(std::shared_ptr<ISystem> system)  = 0;
    virtual void update(Registry& registry, float deltaTime) = 0;
    virtual void stop()                                      = 0;
};
