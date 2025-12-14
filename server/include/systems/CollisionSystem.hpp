#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <array>
#include <vector>

struct Collision
{
    EntityId a;
    EntityId b;
};

class CollisionSystem
{
  public:
    std::vector<Collision> detect(Registry& registry) const;
};
