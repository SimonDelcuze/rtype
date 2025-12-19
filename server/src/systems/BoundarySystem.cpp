#include "systems/BoundarySystem.hpp"

#include "components/RespawnTimerComponent.hpp"

#include <algorithm>

void BoundarySystem::update(Registry& registry) const
{
    for (EntityId id : registry.view<TransformComponent, BoundaryComponent>()) {
        if (!registry.isAlive(id))
            continue;
        if (registry.has<RespawnTimerComponent>(id))
            continue;
        auto& transform    = registry.get<TransformComponent>(id);
        const auto& bounds = registry.get<BoundaryComponent>(id);

        transform.x = std::clamp(transform.x, bounds.minX, bounds.maxX);
        transform.y = std::clamp(transform.y, bounds.minY, bounds.maxY);
    }
}
