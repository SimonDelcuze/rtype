#include "systems/DamageSystem.hpp"

#include <iostream>

DamageSystem::DamageSystem(EventBus& bus) : bus_(bus) {}

void DamageSystem::apply(Registry& registry, const std::vector<Collision>& collisions)
{
    for (const auto& col : collisions) {
        EntityId first  = col.a;
        EntityId second = col.b;
        
        auto handleMissile = [&](EntityId missileId, EntityId targetId) {
            if (!registry.isAlive(missileId) || !registry.isAlive(targetId))
                return;
            if (!registry.has<MissileComponent>(missileId))
                return;
            if (!registry.has<HealthComponent>(targetId))
                return;
            auto& m          = registry.get<MissileComponent>(missileId);
            auto& h          = registry.get<HealthComponent>(targetId);
            std::int32_t dmg = m.damage;
            if (registry.has<MissileComponent>(targetId)) {
                dmg = std::max(dmg, registry.get<MissileComponent>(targetId).damage);
            }
            std::int32_t before = h.current;
            h.damage(dmg);
            DamageEvent ev{};
            ev.attacker  = registry.has<OwnershipComponent>(missileId)
                                   ? registry.get<OwnershipComponent>(missileId).ownerId
                                   : missileId;
            ev.target    = targetId;
            ev.amount    = std::min(before, dmg);
            ev.remaining = h.current;
            bus_.emit<DamageEvent>(ev);
        };
        
        auto handleDirectCollision = [&](EntityId entityA, EntityId entityB) {
            if (!registry.isAlive(entityA) || !registry.isAlive(entityB))
                return;
            if (!registry.has<TagComponent>(entityA) || !registry.has<TagComponent>(entityB))
                return;
            if (!registry.has<HealthComponent>(entityB))
                return;
            
            auto& tagA = registry.get<TagComponent>(entityA);
            auto& tagB = registry.get<TagComponent>(entityB);
            
            if (tagA.hasTag(EntityTag::Player) && tagB.hasTag(EntityTag::Enemy)) {
                auto& h = registry.get<HealthComponent>(entityB);
                std::int32_t dmg = 25;
                std::int32_t before = h.current;
                h.damage(dmg);
                std::cout << "[DamageSystem] Player (ID:" << entityA << ") damaged Enemy (ID:" << entityB 
                          << ") for " << dmg << " damage. Health: " << before << " -> " << h.current << "\n";
                DamageEvent ev{};
                ev.attacker  = entityA;
                ev.target    = entityB;
                ev.amount    = std::min(before, dmg);
                ev.remaining = h.current;
                bus_.emit<DamageEvent>(ev);
            }
            else if (tagA.hasTag(EntityTag::Enemy) && tagB.hasTag(EntityTag::Player)) {
                auto& h = registry.get<HealthComponent>(entityB);
                std::int32_t dmg = 10;
                std::int32_t before = h.current;
                h.damage(dmg);
                std::cout << "[DamageSystem] Enemy (ID:" << entityA << ") damaged Player (ID:" << entityB 
                          << ") for " << dmg << " damage. Health: " << before << " -> " << h.current << "\n";
                DamageEvent ev{};
                ev.attacker  = entityA;
                ev.target    = entityB;
                ev.amount    = std::min(before, dmg);
                ev.remaining = h.current;
                bus_.emit<DamageEvent>(ev);
            }
        };
        
        handleMissile(first, second);
        handleMissile(second, first);
        
        if (!registry.has<MissileComponent>(first) && !registry.has<MissileComponent>(second)) {
            handleDirectCollision(first, second);
            handleDirectCollision(second, first);
        }
    }
    bus_.process();
}
