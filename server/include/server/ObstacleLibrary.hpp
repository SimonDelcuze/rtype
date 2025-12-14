#pragma once

#include "systems/ObstacleSpawnSystem.hpp"

namespace Obstacles
{
    inline ObstacleSpawn at(float time, float x, float y, const HitboxComponent& hitbox, std::int32_t health = 50,
                            float speedX = -50.0F, std::uint16_t typeId = 9)
    {
        ObstacleSpawn spawn{};
        spawn.time   = time;
        spawn.x      = x;
        spawn.y      = y;
        spawn.anchor = ObstacleAnchor::Absolute;
        spawn.health = health;
        spawn.typeId = typeId;
        spawn.speedX = speedX;
        spawn.hitbox = hitbox;
        return spawn;
    }

    inline ObstacleSpawn top(float time, float x, const HitboxComponent& hitbox, std::int32_t health = 50,
                             float margin = 0.0F, float speedX = -50.0F, std::uint16_t typeId = 9)
    {
        ObstacleSpawn spawn = at(time, x, 0.0F, hitbox, health, speedX, typeId);
        spawn.anchor        = ObstacleAnchor::Top;
        spawn.margin        = margin;
        return spawn;
    }

    inline ObstacleSpawn bottom(float time, float x, const HitboxComponent& hitbox, std::int32_t health = 50,
                                float margin = 0.0F, float speedX = -50.0F, std::uint16_t typeId = 9)
    {
        ObstacleSpawn spawn = at(time, x, 0.0F, hitbox, health, speedX, typeId);
        spawn.anchor        = ObstacleAnchor::Bottom;
        spawn.margin        = margin;
        return spawn;
    }
} // namespace Obstacles
