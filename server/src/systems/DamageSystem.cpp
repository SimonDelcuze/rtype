#include "systems/DamageSystem.hpp"

#include "Logger.hpp"

DamageSystem::DamageSystem(EventBus& bus) : bus_(bus) {}

void DamageSystem::apply(Registry& registry, const std::vector<Collision>& collisions)
{
    for (const auto& col : collisions) {
        EntityId first  = col.a;
        EntityId second = col.b;

        applyMissileDamage(registry, first, second);
        applyMissileDamage(registry, second, first);

        applyObstacleCollision(registry, first, second);
        applyObstacleCollision(registry, second, first);

        if (!registry.has<MissileComponent>(first) && !registry.has<MissileComponent>(second)) {
            applyDirectCollisionDamage(registry, first, second);
            applyDirectCollisionDamage(registry, second, first);
        }
    }
    bus_.process();
}

void DamageSystem::applyMissileDamage(Registry& registry, EntityId missileId, EntityId targetId)
{
    if (!registry.isAlive(missileId) || !registry.isAlive(targetId)) {
        return;
    }
    if (!registry.has<MissileComponent>(missileId)) {
        return;
    }
    if (!registry.has<HealthComponent>(targetId)) {
        return;
    }

    auto& missile = registry.get<MissileComponent>(missileId);

    if (registry.has<TagComponent>(targetId)) {
        auto& targetTag = registry.get<TagComponent>(targetId);

        if (missile.fromPlayer && !targetTag.hasTag(EntityTag::Enemy)) {
            return;
        }

        if (!missile.fromPlayer && !targetTag.hasTag(EntityTag::Player)) {
            return;
        }
    }

    auto& h          = registry.get<HealthComponent>(targetId);
    std::int32_t dmg = missile.damage;
    if (registry.has<MissileComponent>(targetId)) {
        dmg = std::max(dmg, registry.get<MissileComponent>(targetId).damage);
    }
    std::int32_t before = h.current;
    h.damage(dmg);

    EntityId attacker =
        registry.has<OwnershipComponent>(missileId) ? registry.get<OwnershipComponent>(missileId).ownerId : missileId;

    Logger::instance().info((missile.fromPlayer ? "Player" : "Enemy") + std::string(" missile (ID:") +
                            std::to_string(missileId) + ") damaged " + (missile.fromPlayer ? "Enemy" : "Player") +
                            " (ID:" + std::to_string(targetId) + ") for " + std::to_string(dmg) +
                            " damage. Health: " + std::to_string(before) + " -> " + std::to_string(h.current));

    emitDamageEvent(attacker, targetId, std::min(before, dmg), h.current);
}

void DamageSystem::applyDirectCollisionDamage(Registry& registry, EntityId entityA, EntityId entityB)
{
    if (!registry.isAlive(entityA) || !registry.isAlive(entityB)) {
        return;
    }
    if (!registry.has<TagComponent>(entityA) || !registry.has<TagComponent>(entityB)) {
        return;
    }
    if (!registry.has<HealthComponent>(entityB)) {
        return;
    }

    auto& tagA = registry.get<TagComponent>(entityA);
    auto& tagB = registry.get<TagComponent>(entityB);

    if (tagA.hasTag(EntityTag::Player) && tagB.hasTag(EntityTag::Enemy)) {
        auto& h             = registry.get<HealthComponent>(entityB);
        std::int32_t dmg    = 25;
        std::int32_t before = h.current;
        h.damage(dmg);
        Logger::instance().info("Player (ID:" + std::to_string(entityA) +
                                ") damaged Enemy (ID:" + std::to_string(entityB) + ") for " + std::to_string(dmg) +
                                " damage. Health: " + std::to_string(before) + " -> " + std::to_string(h.current));
        emitDamageEvent(entityA, entityB, std::min(before, dmg), h.current);
    } else if (tagA.hasTag(EntityTag::Enemy) && tagB.hasTag(EntityTag::Player)) {
        auto& h             = registry.get<HealthComponent>(entityB);
        std::int32_t dmg    = 10;
        std::int32_t before = h.current;
        h.damage(dmg);
        Logger::instance().info("Enemy (ID:" + std::to_string(entityA) +
                                ") damaged Player (ID:" + std::to_string(entityB) + ") for " + std::to_string(dmg) +
                                " damage. Health: " + std::to_string(before) + " -> " + std::to_string(h.current));
        emitDamageEvent(entityA, entityB, std::min(before, dmg), h.current);
    }
}

void DamageSystem::applyObstacleCollision(Registry& registry, EntityId obstacleId, EntityId otherId)
{
    if (!registry.isAlive(obstacleId) || !registry.isAlive(otherId)) {
        return;
    }
    if (!registry.has<TagComponent>(obstacleId) || !registry.has<TagComponent>(otherId)) {
        return;
    }

    const auto& obstacleTag = registry.get<TagComponent>(obstacleId);
    if (!obstacleTag.hasTag(EntityTag::Obstacle)) {
        return;
    }

    const auto& otherTag = registry.get<TagComponent>(otherId);
    if (!otherTag.hasTag(EntityTag::Player)) {
        return;
    }
    if (!registry.has<HealthComponent>(otherId)) {
        return;
    }

    auto& health      = registry.get<HealthComponent>(otherId);
    std::int32_t prev = health.current;
    if (prev <= 0) {
        return;
    }
    health.damage(prev);

    Logger::instance().info("Obstacle (ID:" + std::to_string(obstacleId) +
                            ") destroyed Player (ID:" + std::to_string(otherId) + ")");
    emitDamageEvent(obstacleId, otherId, prev, health.current);
}

void DamageSystem::emitDamageEvent(EntityId attacker, EntityId target, std::int32_t amount, std::int32_t remaining)
{
    DamageEvent ev{};
    ev.attacker  = attacker;
    ev.target    = target;
    ev.amount    = amount;
    ev.remaining = remaining;
    bus_.emit<DamageEvent>(ev);
}
