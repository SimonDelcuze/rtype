#include "systems/PlayerInputSystem.hpp"

#include <cmath>

PlayerInputSystem::PlayerInputSystem(float speed, float missileSpeed, float missileLifetime, std::int32_t missileDamage)
    : speed_(speed), missileSpeed_(missileSpeed), missileLifetime_(missileLifetime), missileDamage_(missileDamage)
{}

void PlayerInputSystem::update(Registry& registry, const std::vector<PlayerCommand>& commands) const
{
    for (const auto& cmd : commands) {
        EntityId id = cmd.playerId;
        if (!registry.isAlive(id))
            continue;
        if (registry.has<HealthComponent>(id) && registry.get<HealthComponent>(id).current <= 0)
            continue;
        if (!registry.has<PlayerInputComponent>(id))
            continue;
        auto& comp = registry.get<PlayerInputComponent>(id);
        if (cmd.sequenceId <= comp.sequenceId)
            continue;
        comp.sequenceId = cmd.sequenceId;
        comp.x          = cmd.x;
        comp.y          = cmd.y;
        comp.angle      = cmd.angle;

        if (registry.has<VelocityComponent>(id)) {
            float dx = 0.0F;
            float dy = 0.0F;
            if (cmd.inputFlags & static_cast<std::uint16_t>(InputFlag::MoveUp))
                dy -= 1.0F;
            if (cmd.inputFlags & static_cast<std::uint16_t>(InputFlag::MoveDown))
                dy += 1.0F;
            if (cmd.inputFlags & static_cast<std::uint16_t>(InputFlag::MoveLeft))
                dx -= 1.0F;
            if (cmd.inputFlags & static_cast<std::uint16_t>(InputFlag::MoveRight))
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

        bool fire = (cmd.inputFlags & static_cast<std::uint16_t>(InputFlag::Fire)) != 0;
        if (fire && registry.has<TransformComponent>(id)) {
            auto& playerTransform = registry.get<TransformComponent>(id);
            float playerX         = playerTransform.x;
            float playerY         = playerTransform.y;
            float dirX            = std::cos(comp.angle);
            float dirY            = std::sin(comp.angle);
            EntityId missile      = registry.createEntity();
            auto& mt              = registry.emplace<TransformComponent>(missile);
            mt.x                  = playerX;
            mt.y                  = playerY + 3.0F;
            mt.rotation           = comp.angle;
            auto& mv              = registry.emplace<VelocityComponent>(missile);
            const int chargeLevel = chargeLevelFromFlags(cmd.inputFlags);
            const float speed     = missileSpeed_ * (1.0F + 0.1F * static_cast<float>(chargeLevel - 1));
            const float lifetime  = missileLifetime_ * (1.0F + 0.1F * static_cast<float>(chargeLevel - 1));
            const std::int32_t dmg =
                static_cast<std::int32_t>(static_cast<float>(missileDamage_) * (1.0F + 0.2F * (chargeLevel - 1)));
            mv.vx = dirX * speed;
            mv.vy = dirY * speed;
            registry.emplace<MissileComponent>(missile, MissileComponent{dmg, lifetime, true, chargeLevel});
            registry.emplace<OwnershipComponent>(missile, OwnershipComponent::create(id, 0));
            registry.emplace<TagComponent>(missile, TagComponent::create(EntityTag::Projectile));
            registry.emplace<HitboxComponent>(missile, HitboxComponent::create(20.0F, 20.0F, 0.0F, 0.0F, true));
        }
    }
}

int PlayerInputSystem::chargeLevelFromFlags(std::uint16_t flags) const
{
    if (flags & static_cast<std::uint16_t>(InputFlag::Charge5))
        return 5;
    if (flags & static_cast<std::uint16_t>(InputFlag::Charge4))
        return 4;
    if (flags & static_cast<std::uint16_t>(InputFlag::Charge3))
        return 3;
    if (flags & static_cast<std::uint16_t>(InputFlag::Charge2))
        return 2;
    return 1;
}
