#include "systems/InterpolationSystem.hpp"

#include "components/InterpolationComponent.hpp"
#include "components/TransformComponent.hpp"

void InterpolationSystem::update(Registry& registry, float deltaTime)
{
    for (EntityId entity = 0; entity < registry.entityCount(); ++entity) {
        if (!registry.isAlive(entity)) {
            continue;
        }

        if (!registry.has<InterpolationComponent>(entity) || !registry.has<TransformComponent>(entity)) {
            continue;
        }

        auto& interp    = registry.get<InterpolationComponent>(entity);
        auto& transform = registry.get<TransformComponent>(entity);

        if (!interp.enabled) {
            continue;
        }

        interp.elapsedTime += deltaTime;

        switch (interp.mode) {
            case InterpolationMode::Linear: {
                float t     = clamp(interp.elapsedTime / interp.interpolationTime, 0.0F, 1.0F);
                transform.x = lerp(interp.previousX, interp.targetX, t);
                transform.y = lerp(interp.previousY, interp.targetY, t);
                break;
            }

            case InterpolationMode::Extrapolate: {
                if (interp.elapsedTime <= interp.interpolationTime) {
                    float t     = interp.elapsedTime / interp.interpolationTime;
                    transform.x = lerp(interp.previousX, interp.targetX, t);
                    transform.y = lerp(interp.previousY, interp.targetY, t);
                } else {
                    float extraTime = interp.elapsedTime - interp.interpolationTime;
                    transform.x     = interp.targetX + interp.velocityX * extraTime;
                    transform.y     = interp.targetY + interp.velocityY * extraTime;
                }
                break;
            }

            case InterpolationMode::None:
            default:
                transform.x = interp.targetX;
                transform.y = interp.targetY;
                break;
        }
    }
}

float InterpolationSystem::lerp(float start, float end, float t) const
{
    return start + (end - start) * t;
}

float InterpolationSystem::clamp(float value, float min, float max) const
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}
