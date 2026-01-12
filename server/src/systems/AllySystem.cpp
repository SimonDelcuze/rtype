#include "systems/AllySystem.hpp"

#include "Logger.hpp"
#include "components/AllyComponent.hpp"
#include "components/Components.hpp"

#include <vector>

void AllySystem::update(Registry& registry, float deltaTime)
{
    std::vector<EntityId> toDestroy;

    for (EntityId allyId : registry.view<AllyComponent, TransformComponent>()) {
        if (!registry.isAlive(allyId))
            continue;

        auto& ally = registry.get<AllyComponent>(allyId);

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
            if (ownership.ownerId == ally.ownerId) {
                ownerId    = playerId;
                ownerFound = true;
                break;
            }
        }

        if (!ownerFound) {
            toDestroy.push_back(allyId);
            continue;
        }

        const auto& ownerTransform = registry.get<TransformComponent>(ownerId);

        if (ownerTransform.y < -1000.0F) {
            continue;
        }

        auto& allyTransform = registry.get<TransformComponent>(allyId);
        allyTransform.x     = ownerTransform.x;
        allyTransform.y     = ownerTransform.y + kVerticalOffset;

        ally.shootTimer += deltaTime;
        if (ally.shootTimer >= AllyComponent::kShootInterval) {
            ally.shootTimer  = 0.0F;
            EntityId missile = registry.createEntity();

            auto& mt = registry.emplace<TransformComponent>(missile);
            mt.x     = allyTransform.x + 20.0F;
            mt.y     = allyTransform.y + 3.0F;

            auto& mv = registry.emplace<VelocityComponent>(missile);
            mv.vx    = kMissileSpeed;
            mv.vy    = 0.0F;

            registry.emplace<MissileComponent>(missile, MissileComponent{kMissileDamage, kMissileLifetime, true, 1});

            registry.emplace<OwnershipComponent>(missile, OwnershipComponent::create(ally.ownerId, 0));

            registry.emplace<TagComponent>(missile, TagComponent::create(EntityTag::Projectile));
            registry.emplace<HitboxComponent>(missile, HitboxComponent::create(20.0F, 20.0F, 0.0F, 0.0F, true));

            Logger::instance().info("[Ally] Ally (owner=" + std::to_string(ally.ownerId) + ") fired missile at (" +
                                    std::to_string(mt.x) + ", " + std::to_string(mt.y) + ")");
        }
    }

    for (EntityId id : toDestroy) {
        if (registry.isAlive(id)) {
            Logger::instance().info("[Ally] Destroying orphaned ally entity " + std::to_string(id));
            registry.destroyEntity(id);
        }
    }
}
