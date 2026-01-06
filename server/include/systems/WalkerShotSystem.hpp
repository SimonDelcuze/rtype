#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

class WalkerShotSystem
{
  public:
    WalkerShotSystem() = default;
    void update(Registry& registry, float deltaTime) const;
};
