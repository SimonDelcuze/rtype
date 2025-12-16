#include "server/EntityTypeResolver.hpp"

#include "components/OwnershipComponent.hpp"

#include <algorithm>

namespace
{
    std::uint16_t typeForProjectile(const Registry& registry, EntityId id)
    {
        if (registry.has<OwnershipComponent>(id)) {
            EntityId owner = registry.get<OwnershipComponent>(id).ownerId;
            if (registry.isAlive(owner) && registry.has<TagComponent>(owner)) {
                if (registry.get<TagComponent>(owner).hasTag(EntityTag::Enemy)) {
                    return 15;
                }
            }
        }

        int charge = 1;
        if (registry.has<MissileComponent>(id)) {
            charge = std::clamp(registry.get<MissileComponent>(id).chargeLevel, 1, 5);
        }
        switch (charge) {
            case 1:
                return 3;
            case 2:
                return 4;
            case 3:
                return 5;
            case 4:
                return 6;
            case 5:
            default:
                return 8;
        }
    }
} // namespace

std::uint16_t resolveEntityType(const Registry& registry, EntityId id)
{
    if (registry.has<RenderTypeComponent>(id)) {
        return registry.get<RenderTypeComponent>(id).typeId;
    }
    if (registry.has<TagComponent>(id)) {
        const auto& tag = registry.get<TagComponent>(id);
        if (tag.hasTag(EntityTag::Player)) {
            return 1;
        }
        if (tag.hasTag(EntityTag::Projectile)) {
            return typeForProjectile(registry, id);
        }
        if (tag.hasTag(EntityTag::Obstacle)) {
            return 9;
        }
    }
    return 2;
}
