#include "systems/ShieldSystem.hpp"

#include "Logger.hpp"
#include "components/Components.hpp"
#include "components/ShieldComponent.hpp"

#include <vector>

void ShieldSystem::update(Registry& registry, float deltaTime)
{
    (void) deltaTime;
    std::vector<EntityId> toDestroy;

    for (EntityId shieldId : registry.view<ShieldComponent, TransformComponent>()) {
        if (!registry.isAlive(shieldId))
            continue;

        auto& shield = registry.get<ShieldComponent>(shieldId);

        EntityId ownerId = 0;
        bool ownerFound  = false;
        for (EntityId playerId : registry.view<TagComponent, TransformComponent>()) {
            if (!registry.isAlive(playerId))
                continue;
            const auto& tag = registry.get<TagComponent>(playerId);
            if (!tag.hasTag(EntityTag::Player))
                continue;
            if (!registry.has<OwnershipComponent>(playerId))
                continue;
            const auto& ownership = registry.get<OwnershipComponent>(playerId);
            if (ownership.ownerId == shield.ownerId) {
                ownerId    = playerId;
                ownerFound = true;
                break;
            }
        }

        if (!ownerFound) {
            toDestroy.push_back(shieldId);
            continue;
        }

        const auto& ownerTransform = registry.get<TransformComponent>(ownerId);

        if (ownerTransform.y < -1000.0F) {
            continue;
        }

        auto& shieldTransform = registry.get<TransformComponent>(shieldId);
        shieldTransform.x     = ownerTransform.x + kHorizontalOffset;
        shieldTransform.y     = ownerTransform.y;
    }

    for (EntityId id : toDestroy) {
        if (registry.isAlive(id)) {
            Logger::instance().info("[Shield] Destroying orphaned shield entity " + std::to_string(id));
            registry.destroyEntity(id);
        }
    }
}
