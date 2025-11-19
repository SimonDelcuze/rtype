#include "components/MovementComponent.hpp"
#include "ecs/Registry.hpp"

#include <gtest/gtest.h>

TEST(MovementComponent, StoresMovementParameters)
{
    Registry registry;
    const EntityId entity      = registry.createEntity();
    const float speed          = 120.0F;
    const float amplitude      = 15.0F;
    const float frequency      = 2.0F;
    const MovementComponent mc = MovementComponent::zigzag(speed, amplitude, frequency);
    registry.emplace<MovementComponent>(entity, mc);

    ASSERT_TRUE(registry.has<MovementComponent>(entity));
    const auto& stored = registry.get<MovementComponent>(entity);
    EXPECT_EQ(stored.pattern, MovementPattern::Zigzag);
    EXPECT_FLOAT_EQ(stored.speed, speed);
    EXPECT_FLOAT_EQ(stored.amplitude, amplitude);
    EXPECT_FLOAT_EQ(stored.frequency, frequency);
    EXPECT_FLOAT_EQ(stored.phase, 0.0F);
}

TEST(MovementComponent, DefaultConstructorIsLinear)
{
    const MovementComponent component;
    EXPECT_EQ(component.pattern, MovementPattern::Linear);
    EXPECT_FLOAT_EQ(component.speed, 0.0F);
    EXPECT_FLOAT_EQ(component.amplitude, 0.0F);
    EXPECT_FLOAT_EQ(component.frequency, 0.0F);
    EXPECT_FLOAT_EQ(component.phase, 0.0F);
}

TEST(MovementComponent, LinearFactorySetsSpeedOnly)
{
    const auto component = MovementComponent::linear(200.0F);
    EXPECT_EQ(component.pattern, MovementPattern::Linear);
    EXPECT_FLOAT_EQ(component.speed, 200.0F);
    EXPECT_FLOAT_EQ(component.amplitude, 0.0F);
    EXPECT_FLOAT_EQ(component.frequency, 0.0F);
    EXPECT_FLOAT_EQ(component.phase, 0.0F);
}

TEST(MovementComponent, SineFactoryAppliesAllParameters)
{
    const float speed     = 90.0F;
    const float amplitude = 7.5F;
    const float frequency = 3.25F;
    const float phase     = 1.2F;
    const auto component  = MovementComponent::sine(speed, amplitude, frequency, phase);

    EXPECT_EQ(component.pattern, MovementPattern::Sine);
    EXPECT_FLOAT_EQ(component.speed, speed);
    EXPECT_FLOAT_EQ(component.amplitude, amplitude);
    EXPECT_FLOAT_EQ(component.frequency, frequency);
    EXPECT_FLOAT_EQ(component.phase, phase);
}
