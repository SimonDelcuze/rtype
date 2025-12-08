#include "systems/CollisionSystem.hpp"

#include <cmath>
#include <iostream>

std::optional<std::array<float, 4>> CollisionSystem::buildAabb(const TransformComponent& t,
                                                               const HitboxComponent& h) const
{
    if (!h.isActive || h.width <= 0.0F || h.height <= 0.0F)
        return std::nullopt;
    if (!std::isfinite(t.x) || !std::isfinite(t.y) || !std::isfinite(h.offsetX) || !std::isfinite(h.offsetY) ||
        !std::isfinite(h.width) || !std::isfinite(h.height))
        return std::nullopt;
    float left   = t.x + h.offsetX - h.width / 2.0F;
    float right  = t.x + h.offsetX + h.width / 2.0F;
    float top    = t.y + h.offsetY - h.height / 2.0F;
    float bottom = t.y + h.offsetY + h.height / 2.0F;
    return std::array<float, 4>{left, right, top, bottom};
}

bool CollisionSystem::overlaps(const std::array<float, 4>& a, const std::array<float, 4>& b) const
{
    bool separated = a[1] < b[0] || a[0] > b[1] || a[3] < b[2] || a[2] > b[3];
    return !separated;
}

std::vector<Collision> CollisionSystem::detect(Registry& registry) const
{
    std::vector<EntityId> ids;
    std::vector<std::array<float, 4>> aabbs;
    ids.reserve(registry.entityCount());
    aabbs.reserve(registry.entityCount());

    for (EntityId id : registry.view<TransformComponent, HitboxComponent>()) {
        if (!registry.isAlive(id))
            continue;
        auto& t   = registry.get<TransformComponent>(id);
        auto& h   = registry.get<HitboxComponent>(id);
        auto aabb = buildAabb(t, h);
        if (!aabb)
            continue;

        ids.push_back(id);
        aabbs.push_back(*aabb);
    }

    std::vector<Collision> out;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        for (std::size_t j = i + 1; j < ids.size(); ++j) {
            if (overlaps(aabbs[i], aabbs[j])) {
                out.push_back(Collision{ids[i], ids[j]});
            }
        }
    }
    return out;
}
