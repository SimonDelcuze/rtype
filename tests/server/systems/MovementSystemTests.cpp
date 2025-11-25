#include "systems/MovementSystem.hpp"

#include <gtest/gtest.h>
#include <limits>

TEST(MovementSystem, MovesEntityByVelocityAndDt)
{
    Registry registry;
    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    auto& v    = registry.emplace<VelocityComponent>(e);
    t.x        = 1.0F;
    t.y        = 2.0F;
    v.vx       = 10.0F;
    v.vy       = -5.0F;

    MovementSystem sys;
    sys.update(registry, 0.5F);

    EXPECT_FLOAT_EQ(t.x, 1.0F + 10.0F * 0.5F);
    EXPECT_FLOAT_EQ(t.y, 2.0F + -5.0F * 0.5F);
}

TEST(MovementSystem, SkipsNonFiniteVelocity)
{
    Registry registry;
    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    auto& v    = registry.emplace<VelocityComponent>(e);
    t.x        = 0.0F;
    t.y        = 0.0F;
    v.vx       = std::numeric_limits<float>::infinity();
    v.vy       = 1.0F;

    MovementSystem sys;
    sys.update(registry, 1.0F);

    EXPECT_FLOAT_EQ(t.x, 0.0F);
    EXPECT_FLOAT_EQ(t.y, 0.0F);
}

TEST(MovementSystem, DoesNotMoveDeadEntities)
{
    Registry registry;
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e);
    registry.emplace<VelocityComponent>(e);
    auto& v     = registry.get<VelocityComponent>(e);
    v.vx        = 7.0F;
    v.vy        = 8.0F;
    auto& tInit = registry.get<TransformComponent>(e);
    tInit.x     = 3.0F;
    tInit.y     = 4.0F;

    registry.destroyEntity(e);

    MovementSystem sys;
    sys.update(registry, 1.0F);

    EXPECT_FLOAT_EQ(tInit.x, 3.0F);
    EXPECT_FLOAT_EQ(tInit.y, 4.0F);
}

TEST(MovementSystem, RotationUnchanged)
{
    Registry registry;
    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    auto& v    = registry.emplace<VelocityComponent>(e);
    t.rotation = 1.25F;
    v.vx       = 5.0F;

    MovementSystem sys;
    sys.update(registry, 2.0F);

    EXPECT_FLOAT_EQ(t.rotation, 1.25F);
}

TEST(MovementSystem, MultipleEntitiesIndependent)
{
    Registry registry;
    EntityId p       = registry.createEntity();
    EntityId m       = registry.createEntity();
    EntityId missile = registry.createEntity();

    registry.emplace<TransformComponent>(p);
    registry.emplace<TransformComponent>(m);
    registry.emplace<TransformComponent>(missile);

    registry.emplace<VelocityComponent>(p);
    registry.emplace<VelocityComponent>(m);
    registry.emplace<VelocityComponent>(missile);

    auto& pv = registry.get<VelocityComponent>(p);
    pv.vx    = 2.0F;
    pv.vy    = 0.0F;

    auto& mv = registry.get<VelocityComponent>(m);
    mv.vx    = 0.0F;
    mv.vy    = -3.0F;

    auto& tv = registry.get<VelocityComponent>(missile);
    tv.vx    = 1.0F;
    tv.vy    = 1.0F;

    MovementSystem sys;
    sys.update(registry, 1.0F);

    auto& pt = registry.get<TransformComponent>(p);
    auto& mt = registry.get<TransformComponent>(m);
    auto& tt = registry.get<TransformComponent>(missile);

    EXPECT_FLOAT_EQ(pt.x, 2.0F);
    EXPECT_FLOAT_EQ(pt.y, 0.0F);
    EXPECT_FLOAT_EQ(mt.x, 0.0F);
    EXPECT_FLOAT_EQ(mt.y, -3.0F);
    EXPECT_FLOAT_EQ(tt.x, 1.0F);
    EXPECT_FLOAT_EQ(tt.y, 1.0F);
}

TEST(MovementSystem, ZeroVelocityNoMovement)
{
    Registry registry;
    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    registry.emplace<VelocityComponent>(e);
    t.x = -5.0F;
    t.y = 7.0F;

    MovementSystem sys;
    sys.update(registry, 2.0F);

    EXPECT_FLOAT_EQ(t.x, -5.0F);
    EXPECT_FLOAT_EQ(t.y, 7.0F);
}

TEST(MovementSystem, DifferentDeltaTimes)
{
    Registry registry;
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e);
    registry.emplace<VelocityComponent>(e);
    auto& v = registry.get<VelocityComponent>(e);
    v.vx    = 4.0F;
    v.vy    = 6.0F;

    MovementSystem sys;
    sys.update(registry, 0.25F);
    auto& t = registry.get<TransformComponent>(e);
    EXPECT_FLOAT_EQ(t.x, 1.0F);
    EXPECT_FLOAT_EQ(t.y, 1.5F);
    sys.update(registry, 0.5F);
    EXPECT_FLOAT_EQ(t.x, 1.0F + 4.0F * 0.5F);
    EXPECT_FLOAT_EQ(t.y, 1.5F + 6.0F * 0.5F);
}
