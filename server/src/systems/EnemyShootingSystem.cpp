#include "systems/EnemyShootingSystem.hpp"

#include "Logger.hpp"

#include <cmath>

void EnemyShootingSystem::update(Registry& registry, float deltaTime)
{
    std::vector<EntityId> enemies;

    for (EntityId id : registry.view<EnemyShootingComponent, TransformComponent, TagComponent>()) {
        if (!registry.isAlive(id))
            continue;

        auto& tag = registry.get<TagComponent>(id);
        if (!tag.hasTag(EntityTag::Enemy))
            continue;

        enemies.push_back(id);
    }

    for (EntityId id : enemies) {
        if (!registry.isAlive(id))
            continue;

        auto& shooting  = registry.get<EnemyShootingComponent>(id);
        auto& transform = registry.get<TransformComponent>(id);

        shooting.timeSinceLastShot += deltaTime;

        if (shooting.timeSinceLastShot >= shooting.shootInterval) {
            shooting.timeSinceLastShot = 0.0F;

            Logger::instance().info("Enemy " + std::to_string(id) + " firing projectile at (" +
                                    std::to_string(transform.x) + ", " + std::to_string(transform.y) + ")");

            EntityId projectile = registry.createEntity();

            auto& pt    = registry.emplace<TransformComponent>(projectile);
            pt.x        = transform.x;
            pt.y        = transform.y;
            pt.rotation = 3.14159F;

            auto& pv = registry.emplace<VelocityComponent>(projectile);
            pv.vx    = -shooting.projectileSpeed;
            pv.vy    = 0.0F;

            registry.emplace<MissileComponent>(
                projectile, MissileComponent{shooting.projectileDamage, shooting.projectileLifetime, false});

            registry.emplace<OwnershipComponent>(projectile, OwnershipComponent::create(id, 0));

            registry.emplace<TagComponent>(projectile, TagComponent::create(EntityTag::Projectile));

            registry.emplace<HitboxComponent>(projectile, HitboxComponent::create(20.0F, 20.0F, 0.0F, 0.0F, true));
        }
    }
}
