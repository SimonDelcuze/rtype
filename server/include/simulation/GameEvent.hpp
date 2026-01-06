#pragma once

#include <cstdint>
#include <variant>

struct EntitySpawnedEvent
{
    std::uint32_t entityId;
    std::uint8_t entityType;
    float posX;
    float posY;
};

struct EntityDestroyedEvent
{
    std::uint32_t entityId;
};

struct CollisionEvent
{
    std::uint32_t entityA;
    std::uint32_t entityB;
};

using GameEvent = std::variant<EntitySpawnedEvent, EntityDestroyedEvent, CollisionEvent>;
