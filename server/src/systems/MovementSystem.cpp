#include "systems/MovementSystem.hpp"

#include <cmath>

void MovementSystem::update(Registry& registry, float deltaTime) const
{
    for (EntityId id : registry.view<TransformComponent, VelocityComponent>()) {
        if (!registry.isAlive(id))
            continue;
        auto& t = registry.get<TransformComponent>(id);
        auto& v = registry.get<VelocityComponent>(id);
        if (!std::isfinite(v.vx) || !std::isfinite(v.vy))
            continue;
        t.x += v.vx * deltaTime;
        t.y += v.vy * deltaTime;
    }
}
