#include "systems/DamageSystem.hpp"

DamageSystem::DamageSystem(EventBus& bus) : bus_(bus) {}

void DamageSystem::apply(Registry& registry, const std::vector<Collision>& collisions)
{
    for (const auto& col : collisions) {
        EntityId first  = col.a;
        EntityId second = col.b;
        auto handle     = [&](EntityId missileId, EntityId targetId) {
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
        handle(first, second);
        handle(second, first);
    }
    bus_.process();
}
