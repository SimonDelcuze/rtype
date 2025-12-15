#include "systems/InvincibilitySystem.hpp"

#include "components/InvincibilityComponent.hpp"
#include "ecs/Registry.hpp"

#include <vector>

void InvincibilitySystem::update(Registry& registry, float deltaTime)
{
    std::vector<EntityId> toRemove;

    for (EntityId id : registry.view<InvincibilityComponent>()) {
        auto& inv = registry.get<InvincibilityComponent>(id);

        inv.timeLeft -= deltaTime;

        if (inv.timeLeft <= 0.0F) {
            toRemove.push_back(id);
        }
    }

    for (EntityId id : toRemove) {
        registry.remove<InvincibilityComponent>(id);
    }
}
