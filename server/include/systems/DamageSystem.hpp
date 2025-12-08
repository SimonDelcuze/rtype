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
    void applyMissileDamage(Registry& registry, EntityId missileId, EntityId targetId);
    void applyDirectCollisionDamage(Registry& registry, EntityId entityA, EntityId entityB);
    void emitDamageEvent(EntityId attacker, EntityId target, std::int32_t amount, std::int32_t remaining);

    EventBus& bus_;
};
