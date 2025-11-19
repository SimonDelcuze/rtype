#include "ecs/Registry.hpp"

#include <gtest/gtest.h>

namespace {
    struct Position {
        float x = 0.0F;
        float y = 0.0F;
    };

    struct Health {
        int value = 100;
    };
}
TEST(Registry, CreatesAndReusesEntityIds)
{
    Registry registry;
    const EntityId first = registry.createEntity();
    const EntityId second = registry.createEntity();
    EXPECT_EQ(first, 0u);
    EXPECT_EQ(second, 1u);
    registry.destroyEntity(first);
    const EntityId reused = registry.createEntity();
    EXPECT_EQ(reused, first);
}

TEST(Registry, EmplaceAndGetComponent)
{
    Registry registry;
    const EntityId entity = registry.createEntity();
    auto& position = registry.emplace<Position>(entity, 10.0F, 20.0F);
    EXPECT_FLOAT_EQ(position.x, 10.0F);
    EXPECT_FLOAT_EQ(position.y, 20.0F);

    EXPECT_TRUE(registry.has<Position>(entity));
    const Position& stored = registry.get<Position>(entity);
    EXPECT_FLOAT_EQ(stored.x, 10.0F);
    EXPECT_FLOAT_EQ(stored.y, 20.0F);

    registry.remove<Position>(entity);
    EXPECT_FALSE(registry.has<Position>(entity));
}

TEST(Registry, DestroyEntityRemovesComponents)
{
    Registry registry;
    const EntityId entity = registry.createEntity();
    registry.emplace<Position>(entity, 1.0F, 2.0F);
    registry.emplace<Health>(entity, 50);

    registry.destroyEntity(entity);
    EXPECT_FALSE(registry.isAlive(entity));
    EXPECT_FALSE(registry.has<Position>(entity));
    EXPECT_FALSE(registry.has<Health>(entity));
}

TEST(Registry, EmplaceOnDeadEntityThrows)
{
    Registry registry;
    const EntityId entity = registry.createEntity();
    registry.destroyEntity(entity);
    EXPECT_THROW(registry.emplace<Position>(entity, 1.0F, 2.0F), RegistryError);
}

TEST(Registry, GetFromDeadEntityThrows)
{
    Registry registry;
    const EntityId entity = registry.createEntity();
    registry.emplace<Position>(entity, 5.0F, 6.0F);
    registry.destroyEntity(entity);
    EXPECT_THROW(registry.get<Position>(entity), RegistryError);
}

TEST(Registry, GetMissingComponentThrows)
{
    Registry registry;
    const EntityId entity = registry.createEntity();
    EXPECT_THROW(registry.get<Health>(entity), ComponentNotFoundError);
}

TEST(Registry, RemoveMissingComponentIsSafe)
{
    Registry registry;
    const EntityId entity = registry.createEntity();
    EXPECT_NO_THROW(registry.remove<Health>(entity));
}
