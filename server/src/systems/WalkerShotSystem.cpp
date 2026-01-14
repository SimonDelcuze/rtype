#include "systems/WalkerShotSystem.hpp"

#include <algorithm>

namespace
{
    constexpr std::uint16_t kWalkerTypeId = 21;

    bool isOwnerStillWalker(const Registry& registry, EntityId ownerId)
    {
        if (!registry.isAlive(ownerId)) {
            return false;
        }
        if (!registry.has<TransformComponent>(ownerId)) {
            return false;
        }
        if (!registry.has<RenderTypeComponent>(ownerId)) {
            return false;
        }
        return registry.get<RenderTypeComponent>(ownerId).typeId == kWalkerTypeId;
    }
} // namespace

void WalkerShotSystem::update(Registry& registry, float deltaTime) const
{
    for (EntityId id : registry.view<WalkerShotComponent, TransformComponent>()) {
        if (!registry.isAlive(id))
            continue;

        auto& shot = registry.get<WalkerShotComponent>(id);
        if (shot.ownerId == 0 || !isOwnerStillWalker(registry, shot.ownerId)) {
            if (registry.has<MissileComponent>(id)) {
                registry.get<MissileComponent>(id).lifetime = 0.0F;
            }
            continue;
        }

        shot.elapsed += deltaTime;
        while (shot.elapsed >= shot.tickDuration && shot.tickDuration > 0.0F) {
            shot.elapsed -= shot.tickDuration;
            shot.currentTick++;
        }

        const int totalTicks = shot.ascentTicks + shot.hoverTicks + shot.descendTicks;
        if (totalTicks <= 0) {
            continue;
        }

        const bool finished  = shot.currentTick >= totalTicks;
        const int activeTick = finished ? (totalTicks - 1) : std::max(0, std::min(shot.currentTick, totalTicks - 1));
        const float tickFactor =
            (shot.tickDuration > 0.0F && !finished) ? std::min(shot.elapsed / shot.tickDuration, 1.0F) : 0.0F;

        float offsetY = 0.0F;
        if (activeTick < shot.ascentTicks) {
            const float perTick = shot.apexOffset / static_cast<float>(std::max(shot.ascentTicks, 1));
            offsetY             = -(perTick * (static_cast<float>(activeTick) + tickFactor));
        } else if (activeTick < shot.ascentTicks + shot.hoverTicks) {
            offsetY = -shot.apexOffset;
        } else {
            const int descendTick = activeTick - shot.ascentTicks - shot.hoverTicks;
            const float perTick   = shot.apexOffset / static_cast<float>(std::max(shot.descendTicks, 1));
            offsetY               = -shot.apexOffset + perTick * (static_cast<float>(descendTick) + tickFactor);
        }

        auto& ownerTransform = registry.get<TransformComponent>(shot.ownerId);
        auto& transform      = registry.get<TransformComponent>(id);
        transform.x          = ownerTransform.x + shot.anchorOffsetX;
        transform.y          = ownerTransform.y + shot.anchorOffsetY + offsetY;

        if (finished && registry.has<MissileComponent>(id)) {
            registry.get<MissileComponent>(id).lifetime = 0.0F;
        }
    }
}
