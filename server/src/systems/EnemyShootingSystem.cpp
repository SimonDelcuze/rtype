#include "systems/EnemyShootingSystem.hpp"

#include "Logger.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr std::uint16_t kWalkerTypeId      = 21;
    constexpr std::uint16_t kWalkerShotTypeId  = 22;
    constexpr float kWalkerShotAnchorOffsetX   = 6.0F;
    constexpr float kWalkerShotAnchorOffset    = -10.0F;
    constexpr float kWalkerShotApexOffset      = 200.0F;
    constexpr float kWalkerShotTickDurationSec = 0.16F;

    bool isWalker(const Registry& registry, EntityId id)
    {
        return registry.has<RenderTypeComponent>(id) && registry.get<RenderTypeComponent>(id).typeId == kWalkerTypeId;
    }

    void spawnWalkerShot(Registry& registry, EntityId owner, const TransformComponent& transform,
                         const EnemyShootingComponent& shooting)
    {
        EntityId projectile = registry.createEntity();

        auto& pt = registry.emplace<TransformComponent>(projectile);
        pt.x     = transform.x + kWalkerShotAnchorOffsetX;
        pt.y     = transform.y + kWalkerShotAnchorOffset;

        auto& walkerShot           = registry.emplace<WalkerShotComponent>(
            projectile, WalkerShotComponent::create(owner, kWalkerShotTickDurationSec, kWalkerShotAnchorOffset,
                                                    kWalkerShotApexOffset, kWalkerShotAnchorOffsetX));
        walkerShot.ascentTicks  = 4;
        walkerShot.hoverTicks   = 3;
        walkerShot.descendTicks = 4;

        auto& pv = registry.emplace<VelocityComponent>(projectile);
        pv.vx    = 0.0F;
        pv.vy    = 0.0F;

        const int totalTicks =
            std::max(1, walkerShot.ascentTicks + walkerShot.hoverTicks + walkerShot.descendTicks + 1);
        const float lifetime = kWalkerShotTickDurationSec * static_cast<float>(totalTicks);
        registry.emplace<MissileComponent>(projectile, MissileComponent{shooting.projectileDamage, lifetime, false, 1});

        registry.emplace<OwnershipComponent>(projectile, OwnershipComponent::create(owner, 0));
        registry.emplace<TagComponent>(projectile, TagComponent::create(EntityTag::Projectile));
        registry.emplace<HitboxComponent>(projectile, HitboxComponent::create(20.0F, 20.0F, 0.0F, 0.0F, true));
        registry.emplace<RenderTypeComponent>(projectile, RenderTypeComponent::create(kWalkerShotTypeId));

        Logger::instance().info("[Spawn] Walker " + std::to_string(owner) + " fired special shot at (" +
                                std::to_string(pt.x) + ", " + std::to_string(pt.y) + ")");
    }
} // namespace

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

            if (isWalker(registry, id)) {
                spawnWalkerShot(registry, id, transform, shooting);
            } else {
                Logger::instance().info("[Spawn] Enemy " + std::to_string(id) + " firing projectile at (" +
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
                    projectile, MissileComponent{shooting.projectileDamage, shooting.projectileLifetime, false, 1});

                registry.emplace<OwnershipComponent>(projectile, OwnershipComponent::create(id, 0));

                registry.emplace<TagComponent>(projectile, TagComponent::create(EntityTag::Projectile));

                registry.emplace<HitboxComponent>(projectile, HitboxComponent::create(20.0F, 20.0F, 0.0F, 0.0F, true));
            }
        }
    }
}
