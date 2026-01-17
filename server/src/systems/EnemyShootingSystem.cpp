#include "systems/EnemyShootingSystem.hpp"

#include "Logger.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace
{
    constexpr std::uint16_t kWalkerTypeId      = 21;
    constexpr std::uint16_t kWalkerShotTypeId  = 22;
    constexpr std::uint16_t kBossTypeId        = 20;
    constexpr float kWalkerShotAnchorOffsetX   = 6.0F;
    constexpr float kWalkerShotAnchorOffset    = -10.0F;
    constexpr float kWalkerShotApexOffset      = 200.0F;
    constexpr float kWalkerShotTickDurationSec = 0.16F;

    bool isWalker(const Registry& registry, EntityId id)
    {
        return registry.has<RenderTypeComponent>(id) && registry.get<RenderTypeComponent>(id).typeId == kWalkerTypeId;
    }

    bool isBoss(const Registry& registry, EntityId id)
    {
        return registry.has<RenderTypeComponent>(id) && registry.get<RenderTypeComponent>(id).typeId == kBossTypeId;
    }

    void spawnWalkerShot(Registry& registry, EntityId owner, const TransformComponent& transform,
                         const EnemyShootingComponent& shooting)
    {
        EntityId projectile = registry.createEntity();

        auto& pt = registry.emplace<TransformComponent>(projectile);
        pt.x     = transform.x + kWalkerShotAnchorOffsetX;
        pt.y     = transform.y + kWalkerShotAnchorOffset;

        auto& walkerShot = registry.emplace<WalkerShotComponent>(
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

    void spawnBossRadialShots(Registry& registry, EntityId owner, const TransformComponent& transform,
                              const EnemyShootingComponent& shooting)
    {
        constexpr float kPi  = 3.14159265358979323846F;
        constexpr float kTau = 2.0F * kPi;
        constexpr int kBurstCount = 8;

        static thread_local std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<float> angleDist(0.0F, kTau);

        float originX = transform.x;
        float originY = transform.y;
        if (registry.has<HitboxComponent>(owner)) {
            const auto& hb = registry.get<HitboxComponent>(owner);
            const float sx = (transform.scaleX == 0.0F) ? 1.0F : transform.scaleX;
            const float sy = (transform.scaleY == 0.0F) ? 1.0F : transform.scaleY;
            originX += (hb.offsetX + hb.width * 0.5F) * sx;
            originY += (hb.offsetY + hb.height * 0.5F) * sy;
        }

        for (int i = 0; i < kBurstCount; ++i) {
            const float angle = angleDist(rng);
            const float dirX  = std::cos(angle);
            const float dirY  = std::sin(angle);

            EntityId projectile = registry.createEntity();

            auto& pt    = registry.emplace<TransformComponent>(projectile);
            pt.x        = originX;
            pt.y        = originY;
            pt.rotation = angle;

            auto& pv = registry.emplace<VelocityComponent>(projectile);
            pv.vx    = dirX * shooting.projectileSpeed;
            pv.vy    = dirY * shooting.projectileSpeed;

            registry.emplace<MissileComponent>(
                projectile, MissileComponent{shooting.projectileDamage, shooting.projectileLifetime, false, 1});

            registry.emplace<OwnershipComponent>(projectile, OwnershipComponent::create(owner, 0));
            registry.emplace<TagComponent>(projectile, TagComponent::create(EntityTag::Projectile));
            registry.emplace<HitboxComponent>(projectile, HitboxComponent::create(20.0F, 20.0F, 0.0F, 0.0F, true));
        }
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
            } else if (isBoss(registry, id)) {
                spawnBossRadialShots(registry, id, transform, shooting);
            } else {
                float bestDist2 = std::numeric_limits<float>::max();
                float targetX   = transform.x - 100.0F;
                float targetY   = transform.y;

                for (EntityId playerId : registry.view<TransformComponent, TagComponent>()) {
                    if (!registry.isAlive(playerId))
                        continue;
                    const auto& playerTag = registry.get<TagComponent>(playerId);
                    if (!playerTag.hasTag(EntityTag::Player))
                        continue;
                    const auto& playerTransform = registry.get<TransformComponent>(playerId);
                    float dx                    = playerTransform.x - transform.x;
                    float dy                    = playerTransform.y - transform.y;
                    float dist2                 = dx * dx + dy * dy;
                    if (dist2 < bestDist2) {
                        bestDist2 = dist2;
                        targetX   = playerTransform.x;
                        targetY   = playerTransform.y;
                    }
                }

                float dx   = targetX - transform.x;
                float dy   = targetY - transform.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                float dirX = -1.0F;
                float dirY = 0.0F;
                if (dist > 0.001F) {
                    dirX = dx / dist;
                    dirY = dy / dist;
                }

                Logger::instance().info("[Spawn] Enemy " + std::to_string(id) + " firing projectile at (" +
                                        std::to_string(transform.x) + ", " + std::to_string(transform.y) + ")");

                EntityId projectile = registry.createEntity();

                auto& pt    = registry.emplace<TransformComponent>(projectile);
                pt.x        = transform.x;
                pt.y        = transform.y;
                pt.rotation = std::atan2(dirY, dirX);

                auto& pv = registry.emplace<VelocityComponent>(projectile);
                pv.vx    = dirX * shooting.projectileSpeed;
                pv.vy    = dirY * shooting.projectileSpeed;

                registry.emplace<MissileComponent>(
                    projectile, MissileComponent{shooting.projectileDamage, shooting.projectileLifetime, false, 1});

                registry.emplace<OwnershipComponent>(projectile, OwnershipComponent::create(id, 0));

                registry.emplace<TagComponent>(projectile, TagComponent::create(EntityTag::Projectile));

                registry.emplace<HitboxComponent>(projectile, HitboxComponent::create(20.0F, 20.0F, 0.0F, 0.0F, true));
            }
        }
    }
}
