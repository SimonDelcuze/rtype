#include "systems/PlayerBoundsSystem.hpp"

#include "components/BoundaryComponent.hpp"
#include "components/TagComponent.hpp"

CameraBounds PlayerBoundsSystem::fallbackBounds() const
{
    CameraBounds bounds;
    bounds.minX = 0.0F;
    bounds.maxX = 1246.0F;
    bounds.minY = 0.0F;
    bounds.maxY = 702.0F;
    return bounds;
}

std::optional<CameraBounds> PlayerBoundsSystem::readDefaults(Registry& registry) const
{
    for (EntityId id : registry.view<BoundaryComponent, TagComponent>()) {
        if (!registry.isAlive(id))
            continue;
        const auto& tag = registry.get<TagComponent>(id);
        if (!tag.hasTag(EntityTag::Player))
            continue;
        const auto& b = registry.get<BoundaryComponent>(id);
        CameraBounds bounds;
        bounds.minX = b.minX;
        bounds.maxX = b.maxX;
        bounds.minY = b.minY;
        bounds.maxY = b.maxY;
        return bounds;
    }
    return std::nullopt;
}

void PlayerBoundsSystem::applyBounds(Registry& registry, const CameraBounds& bounds) const
{
    for (EntityId id : registry.view<BoundaryComponent, TagComponent>()) {
        if (!registry.isAlive(id))
            continue;
        const auto& tag = registry.get<TagComponent>(id);
        if (!tag.hasTag(EntityTag::Player))
            continue;
        auto& b = registry.get<BoundaryComponent>(id);
        b.minX  = bounds.minX;
        b.maxX  = bounds.maxX;
        b.minY  = bounds.minY;
        b.maxY  = bounds.maxY;
    }
}

void PlayerBoundsSystem::update(Registry& registry, const std::optional<CameraBounds>& bounds)
{
    if (bounds.has_value()) {
        if (!defaults_.has_value())
            defaults_ = readDefaults(registry);
        active_ = bounds;
        applyBounds(registry, *bounds);
        return;
    }
    if (active_.has_value()) {
        applyBounds(registry, defaults_.value_or(fallbackBounds()));
        active_.reset();
    }
}

void PlayerBoundsSystem::reset()
{
    active_.reset();
    defaults_.reset();
}
