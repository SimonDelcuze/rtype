#include "systems/MonsterMovementSystem.hpp"
#include "systems/MovementSystem.hpp"

#include <cmath>
#include <gtest/gtest.h>
#include <limits>

TEST(MonsterMovementSystem, LinearSetsHorizontalVelocity)
{
    Registry registry;
    EntityId m = registry.createEntity();
    registry.emplace<TransformComponent>(m);
    registry.emplace<VelocityComponent>(m);
    registry.emplace<MovementComponent>(m, MovementComponent::linear(5.0F));

    MonsterMovementSystem sys;
    sys.update(registry, 0.1F);

    auto& v = registry.get<VelocityComponent>(m);
    EXPECT_FLOAT_EQ(v.vx, -5.0F);
    EXPECT_FLOAT_EQ(v.vy, 0.0F);
}

TEST(MonsterMovementSystem, ZigzagAlternatesVerticalVelocity)
{
    Registry registry;
    EntityId m = registry.createEntity();
    registry.emplace<TransformComponent>(m);
    registry.emplace<VelocityComponent>(m);
    registry.emplace<MovementComponent>(m, MovementComponent::zigzag(2.0F, 3.0F, 1.0F));

    MonsterMovementSystem sys;
    sys.update(registry, 0.25F);
    auto& v1 = registry.get<VelocityComponent>(m);
    EXPECT_FLOAT_EQ(v1.vx, -2.0F);
    EXPECT_FLOAT_EQ(v1.vy, 3.0F);

    sys.update(registry, 0.3F);
    auto& v2 = registry.get<VelocityComponent>(m);
    EXPECT_FLOAT_EQ(v2.vx, -2.0F);
    EXPECT_FLOAT_EQ(v2.vy, -3.0F);
}

TEST(MonsterMovementSystem, SineSetsVerticalVelocity)
{
    Registry registry;
    EntityId m = registry.createEntity();
    registry.emplace<TransformComponent>(m);
    registry.emplace<VelocityComponent>(m);
    registry.emplace<MovementComponent>(m, MovementComponent::sine(1.0F, 2.0F, 1.0F, 0.0F));

    MonsterMovementSystem sys;
    sys.update(registry, 0.25F);
    auto& v = registry.get<VelocityComponent>(m);
    EXPECT_NEAR(v.vy, 2.0F * std::sin(3.14159265F * 0.5F), 1e-4);
    EXPECT_FLOAT_EQ(v.vx, -1.0F);
}

TEST(MonsterMovementSystem, ZeroFrequencyStopsVertical)
{
    Registry registry;
    EntityId m1 = registry.createEntity();
    registry.emplace<TransformComponent>(m1);
    registry.emplace<VelocityComponent>(m1);
    registry.emplace<MovementComponent>(m1, MovementComponent::zigzag(2.0F, 3.0F, 0.0F));

    EntityId m2 = registry.createEntity();
    registry.emplace<TransformComponent>(m2);
    registry.emplace<VelocityComponent>(m2);
    registry.emplace<MovementComponent>(m2, MovementComponent::sine(2.0F, 3.0F, 0.0F));

    MonsterMovementSystem sys;
    sys.update(registry, 1.0F);

    auto& v1 = registry.get<VelocityComponent>(m1);
    auto& v2 = registry.get<VelocityComponent>(m2);
    EXPECT_FLOAT_EQ(v1.vy, 0.0F);
    EXPECT_FLOAT_EQ(v2.vy, 0.0F);
}

TEST(MonsterMovementSystem, DrivesMovementSystemIntegration)
{
    Registry registry;
    EntityId m = registry.createEntity();
    registry.emplace<TransformComponent>(m);
    registry.emplace<VelocityComponent>(m);
    registry.emplace<MovementComponent>(m, MovementComponent::linear(4.0F));

    MonsterMovementSystem ai;
    MovementSystem move;

    ai.update(registry, 1.0F);
    move.update(registry, 0.5F);

    auto& t = registry.get<TransformComponent>(m);
    EXPECT_FLOAT_EQ(t.x, -4.0F * 0.5F);
    EXPECT_FLOAT_EQ(t.y, 0.0F);
}

TEST(MonsterMovementSystem, IndependentTimersPerEntity)
{
    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    registry.emplace<TransformComponent>(a);
    registry.emplace<TransformComponent>(b);
    registry.emplace<VelocityComponent>(a);
    registry.emplace<VelocityComponent>(b);
    registry.emplace<MovementComponent>(a, MovementComponent::sine(1.0F, 1.0F, 1.0F, 0.0F));
    registry.emplace<MovementComponent>(b, MovementComponent::sine(1.0F, 1.0F, 1.0F, 1.0F));

    MonsterMovementSystem sys;
    sys.update(registry, 0.25F);

    auto& va = registry.get<VelocityComponent>(a);
    auto& vb = registry.get<VelocityComponent>(b);
    EXPECT_NE(va.vy, vb.vy);
}

TEST(MonsterMovementSystem, SineAccumulatesTime)
{
    Registry registry;
    EntityId m = registry.createEntity();
    registry.emplace<TransformComponent>(m);
    registry.emplace<VelocityComponent>(m);
    registry.emplace<MovementComponent>(m, MovementComponent::sine(1.0F, 1.0F, 1.0F, 0.0F));

    MonsterMovementSystem sys;
    sys.update(registry, 0.1F);
    float vy1 = registry.get<VelocityComponent>(m).vy;
    sys.update(registry, 0.2F);
    float vy2 = registry.get<VelocityComponent>(m).vy;

    EXPECT_NE(vy1, vy2);
    EXPECT_NEAR(vy1, std::sin(0.1F * 6.2831853F), 1e-4);
    EXPECT_NEAR(vy2, std::sin(0.3F * 6.2831853F), 1e-4);
}

TEST(MonsterMovementSystem, ZigzagRepeatsPattern)
{
    Registry registry;
    EntityId m = registry.createEntity();
    registry.emplace<TransformComponent>(m);
    registry.emplace<VelocityComponent>(m);
    registry.emplace<MovementComponent>(m, MovementComponent::zigzag(1.0F, 2.0F, 1.0F));

    MonsterMovementSystem sys;
    sys.update(registry, 0.25F);
    float vy1 = registry.get<VelocityComponent>(m).vy;
    sys.update(registry, 0.25F);
    float vy2 = registry.get<VelocityComponent>(m).vy;
    sys.update(registry, 0.25F);
    float vy3 = registry.get<VelocityComponent>(m).vy;
    sys.update(registry, 0.25F);
    float vy4 = registry.get<VelocityComponent>(m).vy;

    EXPECT_NE(vy1, vy2);
    EXPECT_EQ(vy1, vy4);
    EXPECT_EQ(vy2, vy3);
}

TEST(MonsterMovementSystem, NonFiniteAmplitudeOrFrequencyZeroesVertical)
{
    Registry registry;
    EntityId m1 = registry.createEntity();
    EntityId m2 = registry.createEntity();
    registry.emplace<TransformComponent>(m1);
    registry.emplace<TransformComponent>(m2);
    registry.emplace<VelocityComponent>(m1);
    registry.emplace<VelocityComponent>(m2);
    auto mc1 = MovementComponent::sine(1.0F, std::numeric_limits<float>::quiet_NaN(), 1.0F);
    auto mc2 = MovementComponent::sine(1.0F, 1.0F, std::numeric_limits<float>::infinity());
    registry.emplace<MovementComponent>(m1, mc1);
    registry.emplace<MovementComponent>(m2, mc2);

    MonsterMovementSystem sys;
    sys.update(registry, 0.1F);

    EXPECT_FLOAT_EQ(registry.get<VelocityComponent>(m1).vy, 0.0F);
    EXPECT_FLOAT_EQ(registry.get<VelocityComponent>(m2).vy, 0.0F);
}

TEST(MonsterMovementSystem, SkipsDeadEntities)
{
    Registry registry;
    EntityId m = registry.createEntity();
    registry.emplace<TransformComponent>(m);
    registry.emplace<VelocityComponent>(m);
    registry.emplace<MovementComponent>(m, MovementComponent::linear(3.0F));

    registry.destroyEntity(m);

    MonsterMovementSystem sys;
    EXPECT_NO_THROW(sys.update(registry, 1.0F));
    EXPECT_FALSE(registry.has<VelocityComponent>(m));
}
