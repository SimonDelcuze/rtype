#include "systems/DamageSystem.hpp"

#include "Logger.hpp"
#include "components/InvincibilityComponent.hpp"

DamageSystem::DamageSystem(EventBus& bus) : bus_(bus) {}

void DamageSystem::apply(Registry& registry, const std::vector<Collision>& collisions)
{
    std::vector<EntityId> missilesToDestroy;

    for (const auto& col : collisions) {
        EntityId first  = col.a;
        EntityId second = col.b;

        applyMissileDamage(registry, first, second, missilesToDestroy);
        applyMissileDamage(registry, second, first, missilesToDestroy);

        if (!registry.has<MissileComponent>(first) && !registry.has<MissileComponent>(second)) {
            applyDirectCollisionDamage(registry, first, second);
            applyDirectCollisionDamage(registry, second, first);
        }
    }

    for (EntityId missileId : missilesToDestroy) {
        if (!registry.isAlive(missileId)) {
            continue;
        }
        if (!registry.has<HealthComponent>(missileId)) {
            registry.emplace<HealthComponent>(missileId, HealthComponent::create(0));
            Logger::instance().info("Missile (ID:" + std::to_string(missileId) +
                                    ") marked for destruction after collision");
        }
    }

    bus_.process();
}

void DamageSystem::applyMissileDamage(Registry& registry, EntityId missileId, EntityId targetId,
                                      std::vector<EntityId>& missilesToDestroy)
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
    if (registry.has<InvincibilityComponent>(targetId)) {
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

    missilesToDestroy.push_back(missileId);
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
    if (registry.has<InvincibilityComponent>(entityA) || registry.has<InvincibilityComponent>(entityB)) {
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

void DamageSystem::emitDamageEvent(EntityId attacker, EntityId target, std::int32_t amount, std::int32_t remaining)
{
    DamageEvent ev{};
    ev.attacker  = attacker;
    ev.target    = target;
    ev.amount    = amount;
    ev.remaining = remaining;
    bus_.emit<DamageEvent>(ev);
}
