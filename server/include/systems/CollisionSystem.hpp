#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <optional>
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

  private:
    std::optional<std::array<float, 4>> buildAabb(const TransformComponent& t, const HitboxComponent& h) const;
    bool overlaps(const std::array<float, 4>& a, const std::array<float, 4>& b) const;
};
