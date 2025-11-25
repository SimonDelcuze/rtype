#pragma once

#include "components/Components.hpp"
#include "events/DamageEvent.hpp"
#include "systems/CollisionSystem.hpp"

#include <events/EventBus.hpp>
#include <vector>

class DamageSystem
{
  public:
    explicit DamageSystem(EventBus& bus);
    void apply(Registry& registry, const std::vector<Collision>& collisions);

  private:
    EventBus& bus_;
};
