#pragma once

#include "components/InterpolationComponent.hpp"
#include "ecs/Registry.hpp"

#include <cstdint>

using EntityId = std::uint32_t;

class InterpolationController
{
  public:
    void setTarget(Registry& registry, EntityId entityId, float x, float y);

    void setTargetWithVelocity(Registry& registry, EntityId entityId, float x, float y, float vx, float vy);

    void setMode(Registry& registry, EntityId entityId, InterpolationMode mode);

    void enable(Registry& registry, EntityId entityId);

    void disable(Registry& registry, EntityId entityId);

    void clampToTarget(Registry& registry, EntityId entityId);

    void reset(Registry& registry, EntityId entityId);

    void setInterpolationTime(Registry& registry, EntityId entityId, float time);

    [[nodiscard]] bool isAtTarget(Registry& registry, EntityId entityId, float threshold = 0.01F) const;

    [[nodiscard]] float getProgress(Registry& registry, EntityId entityId) const;
};
