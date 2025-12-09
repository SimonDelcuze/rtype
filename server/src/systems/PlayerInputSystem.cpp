#include "systems/PlayerInputSystem.hpp"

#include <cmath>

PlayerInputSystem::PlayerInputSystem(float speed, float missileSpeed, float missileLifetime, std::int32_t missileDamage)
    : speed_(speed), missileSpeed_(missileSpeed), missileLifetime_(missileLifetime), missileDamage_(missileDamage)
{}

void PlayerInputSystem::update(Registry& registry, const std::vector<ReceivedInput>& inputs) const
{
    for (const auto& ev : inputs) {
        EntityId id = ev.input.playerId;
        if (!registry.isAlive(id))
            continue;
        if (!registry.has<PlayerInputComponent>(id))
            continue;
        auto& comp = registry.get<PlayerInputComponent>(id);
        if (ev.input.sequenceId <= comp.sequenceId)
            continue;
        comp.sequenceId = ev.input.sequenceId;
        comp.x          = ev.input.x;
        comp.y          = ev.input.y;
        comp.angle      = ev.input.angle;

        if (registry.has<VelocityComponent>(id)) {
            float dx = 0.0F;
            float dy = 0.0F;
            if (ev.input.flags & static_cast<std::uint16_t>(InputFlag::MoveUp))
                dy -= 1.0F;
            if (ev.input.flags & static_cast<std::uint16_t>(InputFlag::MoveDown))
                dy += 1.0F;
            if (ev.input.flags & static_cast<std::uint16_t>(InputFlag::MoveLeft))
                dx -= 1.0F;
            if (ev.input.flags & static_cast<std::uint16_t>(InputFlag::MoveRight))
                dx += 1.0F;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > 0.0F) {
                dx /= len;
                dy /= len;
            }
            auto& vel = registry.get<VelocityComponent>(id);
            vel.vx    = dx * speed_;
            vel.vy    = dy * speed_;
        }

        bool fire = (ev.input.flags & static_cast<std::uint16_t>(InputFlag::Fire)) != 0;
        if (fire && registry.has<TransformComponent>(id)) {
            auto& playerTransform = registry.get<TransformComponent>(id);
            float dirX            = std::cos(comp.angle);
            float dirY            = std::sin(comp.angle);
            EntityId missile      = registry.createEntity();
            auto& mt              = registry.emplace<TransformComponent>(missile);
            mt.x                  = playerTransform.x;
            mt.y                  = playerTransform.y;
            mt.rotation           = comp.angle;
            auto& mv              = registry.emplace<VelocityComponent>(missile);
            mv.vx                 = dirX * missileSpeed_;
            mv.vy                 = dirY * missileSpeed_;
            registry.emplace<MissileComponent>(missile, MissileComponent{missileDamage_, missileLifetime_, true});
            registry.emplace<OwnershipComponent>(missile, OwnershipComponent::create(id, 0));
            registry.emplace<TagComponent>(missile, TagComponent::create(EntityTag::Projectile));
        }
    }
}
