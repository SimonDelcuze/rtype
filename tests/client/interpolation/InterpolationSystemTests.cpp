#include "components/InterpolationComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/InterpolationSystem.hpp"

#include <gtest/gtest.h>

class InterpolationSystemTests : public ::testing::Test
{
  protected:
    Registry registry;
    InterpolationSystem system;
};

TEST_F(InterpolationSystemTests, UpdateWithNoEntities)
{
    EXPECT_NO_THROW(system.update(registry, 0.016F));
}

TEST_F(InterpolationSystemTests, LinearInterpolationMidpoint)
{
    EntityId entity = registry.createEntity();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.setTarget(100.0F, 200.0F);
    interp.interpolationTime = 1.0F;
    interp.mode              = InterpolationMode::Linear;

    system.update(registry, 0.5F);

    EXPECT_NEAR(transform.x, 50.0F, 0.01F);
    EXPECT_NEAR(transform.y, 100.0F, 0.01F);
}

TEST_F(InterpolationSystemTests, LinearInterpolationComplete)
{
    EntityId entity = registry.createEntity();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.setTarget(100.0F, 200.0F);
    interp.interpolationTime = 1.0F;
    interp.mode              = InterpolationMode::Linear;

    system.update(registry, 1.5F);

    EXPECT_FLOAT_EQ(transform.x, 100.0F);
    EXPECT_FLOAT_EQ(transform.y, 200.0F);
}

TEST_F(InterpolationSystemTests, LinearInterpolationMultipleUpdates)
{
    EntityId entity = registry.createEntity();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.setTarget(100.0F, 100.0F);
    interp.interpolationTime = 1.0F;
    interp.mode              = InterpolationMode::Linear;

    system.update(registry, 0.25F);
    EXPECT_NEAR(transform.x, 25.0F, 0.01F);

    system.update(registry, 0.25F);
    EXPECT_NEAR(transform.x, 50.0F, 0.01F);

    system.update(registry, 0.25F);
    EXPECT_NEAR(transform.x, 75.0F, 0.01F);

    system.update(registry, 0.25F);
    EXPECT_NEAR(transform.x, 100.0F, 0.01F);
}

TEST_F(InterpolationSystemTests, ExtrapolationWithinWindow)
{
    EntityId entity = registry.createEntity();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.setTargetWithVelocity(100.0F, 100.0F, 50.0F, 50.0F);
    interp.interpolationTime = 1.0F;
    interp.mode              = InterpolationMode::Extrapolate;

    system.update(registry, 0.5F);
    EXPECT_NEAR(transform.x, 50.0F, 0.01F);
    EXPECT_NEAR(transform.y, 50.0F, 0.01F);
}

TEST_F(InterpolationSystemTests, ExtrapolationBeyondWindow)
{
    EntityId entity = registry.createEntity();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.setTargetWithVelocity(100.0F, 100.0F, 50.0F, 50.0F);
    interp.interpolationTime    = 1.0F;
    interp.maxExtrapolationTime = 0.2F;
    interp.mode                 = InterpolationMode::Extrapolate;

    system.update(registry, 1.5F);

    EXPECT_NEAR(transform.x, 110.0F, 0.01F);
    EXPECT_NEAR(transform.y, 110.0F, 0.01F);
}

TEST_F(InterpolationSystemTests, NoneModeSetsToTarget)
{
    EntityId entity = registry.createEntity();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.setTarget(100.0F, 200.0F);
    interp.mode = InterpolationMode::None;

    system.update(registry, 0.5F);

    EXPECT_FLOAT_EQ(transform.x, 100.0F);
    EXPECT_FLOAT_EQ(transform.y, 200.0F);
}

TEST_F(InterpolationSystemTests, DisabledInterpolationSkipped)
{
    EntityId entity = registry.createEntity();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    transform.x = 50.0F;
    transform.y = 50.0F;

    interp.setTarget(100.0F, 100.0F);
    interp.enabled = false;

    system.update(registry, 0.5F);

    EXPECT_FLOAT_EQ(transform.x, 50.0F);
    EXPECT_FLOAT_EQ(transform.y, 50.0F);
}

TEST_F(InterpolationSystemTests, MissingTransformComponentSkipped)
{
    EntityId entity = registry.createEntity();
    registry.emplace<InterpolationComponent>(entity);

    EXPECT_NO_THROW(system.update(registry, 0.016F));
}

TEST_F(InterpolationSystemTests, MissingInterpolationComponentSkipped)
{
    EntityId entity = registry.createEntity();
    registry.emplace<TransformComponent>(entity);

    EXPECT_NO_THROW(system.update(registry, 0.016F));
}

TEST_F(InterpolationSystemTests, DeadEntitySkipped)
{
    EntityId entity = registry.createEntity();
    registry.emplace<TransformComponent>(entity);
    registry.emplace<InterpolationComponent>(entity);

    registry.destroyEntity(entity);

    EXPECT_NO_THROW(system.update(registry, 0.016F));
}

TEST_F(InterpolationSystemTests, MultipleEntitiesProcessed)
{
    EntityId entity1 = registry.createEntity();
    EntityId entity2 = registry.createEntity();

    registry.emplace<TransformComponent>(entity1);
    registry.emplace<TransformComponent>(entity2);
    registry.emplace<InterpolationComponent>(entity1);
    registry.emplace<InterpolationComponent>(entity2);

    auto& interp1 = registry.get<InterpolationComponent>(entity1);
    auto& interp2 = registry.get<InterpolationComponent>(entity2);

    interp1.setTarget(100.0F, 100.0F);
    interp2.setTarget(200.0F, 200.0F);

    interp1.interpolationTime = 1.0F;
    interp2.interpolationTime = 1.0F;

    system.update(registry, 0.5F);

    auto& transform1 = registry.get<TransformComponent>(entity1);
    auto& transform2 = registry.get<TransformComponent>(entity2);

    EXPECT_NEAR(transform1.x, 50.0F, 0.01F);
    EXPECT_NEAR(transform2.x, 100.0F, 0.01F);
}
