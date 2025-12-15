#include "systems/BoundarySystem.hpp"

#include <gtest/gtest.h>

TEST(BoundarySystem, ClampsPositionWithinBounds)
{
    Registry registry;
    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    auto& b    = registry.emplace<BoundaryComponent>(e);
    t.x        = -50.0F;
    t.y        = 300.0F;
    b.minX     = 0.0F;
    b.minY     = 0.0F;
    b.maxX     = 200.0F;
    b.maxY     = 150.0F;

    BoundarySystem sys;
    sys.update(registry);

    EXPECT_FLOAT_EQ(t.x, 0.0F);
    EXPECT_FLOAT_EQ(t.y, 150.0F);
}

TEST(BoundarySystem, DoesNotAffectEntitiesWithoutBoundaryComponent)
{
    Registry registry;
    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    t.x        = -100.0F;
    t.y        = -100.0F;

    BoundarySystem sys;
    sys.update(registry);

    EXPECT_FLOAT_EQ(t.x, -100.0F);
    EXPECT_FLOAT_EQ(t.y, -100.0F);
}

TEST(BoundarySystem, SkipsDeadEntities)
{
    Registry registry;
    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    registry.emplace<BoundaryComponent>(e, BoundaryComponent::create(0.0F, 0.0F, 100.0F, 100.0F));
    t.x = -50.0F;
    t.y = -50.0F;

    registry.destroyEntity(e);

    BoundarySystem sys;
    sys.update(registry);

    EXPECT_FLOAT_EQ(t.x, -50.0F);
    EXPECT_FLOAT_EQ(t.y, -50.0F);
}

TEST(BoundarySystem, AllowsPositionWithinBounds)
{
    Registry registry;
    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    registry.emplace<BoundaryComponent>(e, BoundaryComponent::create(0.0F, 0.0F, 200.0F, 200.0F));
    t.x = 100.0F;
    t.y = 100.0F;

    BoundarySystem sys;
    sys.update(registry);

    EXPECT_FLOAT_EQ(t.x, 100.0F);
    EXPECT_FLOAT_EQ(t.y, 100.0F);
}

TEST(BoundarySystem, ClampsNegativePositions)
{
    Registry registry;
    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    registry.emplace<BoundaryComponent>(e, BoundaryComponent::create(10.0F, 20.0F, 300.0F, 200.0F));
    t.x = -100.0F;
    t.y = -50.0F;

    BoundarySystem sys;
    sys.update(registry);

    EXPECT_FLOAT_EQ(t.x, 10.0F);
    EXPECT_FLOAT_EQ(t.y, 20.0F);
}

TEST(BoundarySystem, ClampsExcessivePositions)
{
    Registry registry;
    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    registry.emplace<BoundaryComponent>(e, BoundaryComponent::create(0.0F, 0.0F, 300.0F, 200.0F));
    t.x = 500.0F;
    t.y = 400.0F;

    BoundarySystem sys;
    sys.update(registry);

    EXPECT_FLOAT_EQ(t.x, 300.0F);
    EXPECT_FLOAT_EQ(t.y, 200.0F);
}

TEST(BoundarySystem, MultipleEntitiesIndependent)
{
    Registry registry;
    EntityId e1 = registry.createEntity();
    EntityId e2 = registry.createEntity();

    registry.emplace<TransformComponent>(e1);
    registry.emplace<TransformComponent>(e2);
    registry.emplace<BoundaryComponent>(e1, BoundaryComponent::create(0.0F, 0.0F, 100.0F, 100.0F));
    registry.emplace<BoundaryComponent>(e2, BoundaryComponent::create(50.0F, 50.0F, 150.0F, 150.0F));

    auto& t1 = registry.get<TransformComponent>(e1);
    auto& t2 = registry.get<TransformComponent>(e2);
    t1.x = -10.0F;
    t1.y = 50.0F;
    t2.x = 200.0F;
    t2.y = 40.0F;

    BoundarySystem sys;
    sys.update(registry);

    EXPECT_FLOAT_EQ(t1.x, 0.0F);
    EXPECT_FLOAT_EQ(t1.y, 50.0F);
    EXPECT_FLOAT_EQ(t2.x, 150.0F);
    EXPECT_FLOAT_EQ(t2.y, 50.0F);
}

