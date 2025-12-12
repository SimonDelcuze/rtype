#pragma once

#include <cstdint>

enum class EntityTypeId : std::uint16_t
{
    Player            = 1,
    Enemy             = 2,
    Projectile        = 3,
    ObstacleSmall     = 4,
    ObstacleMedium    = 5,
    ObstacleLarge     = 6,
    ObstacleTopSmall  = 7,
    ObstacleTopMedium = 8,
    ObstacleTopLarge  = 9
};

constexpr std::uint16_t toTypeId(EntityTypeId type)
{
    return static_cast<std::uint16_t>(type);
}
