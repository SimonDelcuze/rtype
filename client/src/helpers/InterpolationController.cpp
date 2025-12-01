#include "helpers/InterpolationController.hpp"

#include "components/TransformComponent.hpp"

#include <cmath>

void InterpolationController::setTarget(Registry& registry, EntityId entityId, float x, float y)
{
    if (!registry.isAlive(entityId)) {
        return;
    }

    if (!registry.has<InterpolationComponent>(entityId)) {
        return;
    }

    auto& interp = registry.get<InterpolationComponent>(entityId);
    interp.setTarget(x, y);
}

void InterpolationController::setTargetWithVelocity(Registry& registry, EntityId entityId, float x, float y, float vx,
                                                    float vy)
{
    if (!registry.isAlive(entityId)) {
        return;
    }

    if (!registry.has<InterpolationComponent>(entityId)) {
        return;
    }

    auto& interp = registry.get<InterpolationComponent>(entityId);
    interp.setTargetWithVelocity(x, y, vx, vy);
}

void InterpolationController::setMode(Registry& registry, EntityId entityId, InterpolationMode mode)
{
    if (!registry.isAlive(entityId)) {
        return;
    }

    if (!registry.has<InterpolationComponent>(entityId)) {
        return;
    }

    auto& interp = registry.get<InterpolationComponent>(entityId);
    interp.mode  = mode;

    if (mode == InterpolationMode::None) {
        if (registry.has<TransformComponent>(entityId)) {
            auto& transform = registry.get<TransformComponent>(entityId);
            transform.x     = interp.targetX;
            transform.y     = interp.targetY;
        }
    }
}

void InterpolationController::enable(Registry& registry, EntityId entityId)
{
    if (!registry.isAlive(entityId)) {
        return;
    }

    if (!registry.has<InterpolationComponent>(entityId)) {
        return;
    }

    auto& interp   = registry.get<InterpolationComponent>(entityId);
    interp.enabled = true;
}

void InterpolationController::disable(Registry& registry, EntityId entityId)
{
    if (!registry.isAlive(entityId)) {
        return;
    }

    if (!registry.has<InterpolationComponent>(entityId)) {
        return;
    }

    auto& interp   = registry.get<InterpolationComponent>(entityId);
    interp.enabled = false;

    if (registry.has<TransformComponent>(entityId)) {
        auto& transform = registry.get<TransformComponent>(entityId);
        transform.x     = interp.targetX;
        transform.y     = interp.targetY;
    }
}

void InterpolationController::clampToTarget(Registry& registry, EntityId entityId)
{
    if (!registry.isAlive(entityId)) {
        return;
    }

    if (!registry.has<InterpolationComponent>(entityId) || !registry.has<TransformComponent>(entityId)) {
        return;
    }

    auto& interp       = registry.get<InterpolationComponent>(entityId);
    auto& transform    = registry.get<TransformComponent>(entityId);
    transform.x        = interp.targetX;
    transform.y        = interp.targetY;
    interp.elapsedTime = interp.interpolationTime;
    interp.enabled     = false;
}

void InterpolationController::reset(Registry& registry, EntityId entityId)
{
    if (!registry.isAlive(entityId)) {
        return;
    }

    if (!registry.has<InterpolationComponent>(entityId)) {
        return;
    }

    auto& interp       = registry.get<InterpolationComponent>(entityId);
    interp.previousX   = 0.0F;
    interp.previousY   = 0.0F;
    interp.targetX     = 0.0F;
    interp.targetY     = 0.0F;
    interp.elapsedTime = 0.0F;
    interp.velocityX   = 0.0F;
    interp.velocityY   = 0.0F;
    interp.mode        = InterpolationMode::Linear;
    interp.enabled     = true;
}

void InterpolationController::setInterpolationTime(Registry& registry, EntityId entityId, float time)
{
    if (!registry.isAlive(entityId)) {
        return;
    }

    if (!registry.has<InterpolationComponent>(entityId)) {
        return;
    }

    if (time <= 0.0F) {
        return;
    }

    auto& interp             = registry.get<InterpolationComponent>(entityId);
    interp.interpolationTime = time;
}

bool InterpolationController::isAtTarget(Registry& registry, EntityId entityId, float threshold) const
{
    if (!registry.isAlive(entityId)) {
        return false;
    }

    if (!registry.has<InterpolationComponent>(entityId) || !registry.has<TransformComponent>(entityId)) {
        return false;
    }

    const auto& interp     = registry.get<InterpolationComponent>(entityId);
    const auto& transform  = registry.get<TransformComponent>(entityId);
    float dx               = transform.x - interp.targetX;
    float dy               = transform.y - interp.targetY;
    float distanceSquared  = dx * dx + dy * dy;
    float thresholdSquared = threshold * threshold;
    return distanceSquared <= thresholdSquared;
}

float InterpolationController::getProgress(Registry& registry, EntityId entityId) const
{
    if (!registry.isAlive(entityId)) {
        return 0.0F;
    }

    if (!registry.has<InterpolationComponent>(entityId)) {
        return 0.0F;
    }

    const auto& interp = registry.get<InterpolationComponent>(entityId);

    if (interp.interpolationTime <= 0.0F) {
        return 1.0F;
    }

    float progress = interp.elapsedTime / interp.interpolationTime;
    if (progress > 1.0F) {
        return 1.0F;
    }
    if (progress < 0.0F) {
        return 0.0F;
    }
    return progress;
}
