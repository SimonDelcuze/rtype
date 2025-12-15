#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

class BoundarySystem
{
  public:
    void update(Registry& registry) const;
};
