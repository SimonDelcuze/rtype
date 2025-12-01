#include "components/InterpolationComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "helpers/InterpolationController.hpp"

#include <gtest/gtest.h>

class InterpolationControllerTests : public ::testing::Test
{
  protected:
    Registry registry;
    InterpolationController controller;
};

TEST_F(InterpolationControllerTests, SetTargetResetsElapsedTime)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.elapsedTime = 0.5F;
    interp.targetX     = 10.0F;
    interp.targetY     = 20.0F;

    controller.setTarget(registry, entity, 100.0F, 200.0F);

    EXPECT_FLOAT_EQ(interp.elapsedTime, 0.0F);
    EXPECT_FLOAT_EQ(interp.targetX, 100.0F);
    EXPECT_FLOAT_EQ(interp.targetY, 200.0F);
    EXPECT_FLOAT_EQ(interp.previousX, 10.0F);
    EXPECT_FLOAT_EQ(interp.previousY, 20.0F);
}

TEST_F(InterpolationControllerTests, SetTargetWithVelocityResetsElapsedTime)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.elapsedTime = 0.5F;

    controller.setTargetWithVelocity(registry, entity, 100.0F, 200.0F, 5.0F, -3.0F);

    EXPECT_FLOAT_EQ(interp.elapsedTime, 0.0F);
    EXPECT_FLOAT_EQ(interp.targetX, 100.0F);
    EXPECT_FLOAT_EQ(interp.targetY, 200.0F);
    EXPECT_FLOAT_EQ(interp.velocityX, 5.0F);
    EXPECT_FLOAT_EQ(interp.velocityY, -3.0F);
}

TEST_F(InterpolationControllerTests, SetModeChangesMode)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    controller.setMode(registry, entity, InterpolationMode::Extrapolate);
    EXPECT_EQ(interp.mode, InterpolationMode::Extrapolate);

    controller.setMode(registry, entity, InterpolationMode::None);
    EXPECT_EQ(interp.mode, InterpolationMode::None);

    controller.setMode(registry, entity, InterpolationMode::Linear);
    EXPECT_EQ(interp.mode, InterpolationMode::Linear);
}

TEST_F(InterpolationControllerTests, SetModeNoneSnapsToTarget)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);
    auto& transform = registry.emplace<TransformComponent>(entity);

    interp.targetX = 100.0F;
    interp.targetY = 200.0F;
    transform.x    = 10.0F;
    transform.y    = 20.0F;

    controller.setMode(registry, entity, InterpolationMode::None);

    EXPECT_FLOAT_EQ(transform.x, 100.0F);
    EXPECT_FLOAT_EQ(transform.y, 200.0F);
}

TEST_F(InterpolationControllerTests, EnableSetsEnabledTrue)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.enabled = false;

    controller.enable(registry, entity);

    EXPECT_TRUE(interp.enabled);
}

TEST_F(InterpolationControllerTests, DisableSetsEnabledFalse)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.enabled = true;

    controller.disable(registry, entity);

    EXPECT_FALSE(interp.enabled);
}

TEST_F(InterpolationControllerTests, DisableSnapsToTarget)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);
    auto& transform = registry.emplace<TransformComponent>(entity);

    interp.targetX = 100.0F;
    interp.targetY = 200.0F;
    transform.x    = 10.0F;
    transform.y    = 20.0F;

    controller.disable(registry, entity);

    EXPECT_FLOAT_EQ(transform.x, 100.0F);
    EXPECT_FLOAT_EQ(transform.y, 200.0F);
    EXPECT_FALSE(interp.enabled);
}

TEST_F(InterpolationControllerTests, ClampToTargetSnapsAndDisables)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);
    auto& transform = registry.emplace<TransformComponent>(entity);

    interp.targetX           = 100.0F;
    interp.targetY           = 200.0F;
    interp.interpolationTime = 1.0F;
    interp.elapsedTime       = 0.5F;
    transform.x              = 50.0F;
    transform.y              = 100.0F;

    controller.clampToTarget(registry, entity);

    EXPECT_FLOAT_EQ(transform.x, 100.0F);
    EXPECT_FLOAT_EQ(transform.y, 200.0F);
    EXPECT_FLOAT_EQ(interp.elapsedTime, 1.0F);
    EXPECT_FALSE(interp.enabled);
}

TEST_F(InterpolationControllerTests, ResetClearsAllFields)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.previousX   = 10.0F;
    interp.previousY   = 20.0F;
    interp.targetX     = 100.0F;
    interp.targetY     = 200.0F;
    interp.elapsedTime = 0.5F;
    interp.velocityX   = 5.0F;
    interp.velocityY   = -3.0F;
    interp.mode        = InterpolationMode::Extrapolate;
    interp.enabled     = false;

    controller.reset(registry, entity);

    EXPECT_FLOAT_EQ(interp.previousX, 0.0F);
    EXPECT_FLOAT_EQ(interp.previousY, 0.0F);
    EXPECT_FLOAT_EQ(interp.targetX, 0.0F);
    EXPECT_FLOAT_EQ(interp.targetY, 0.0F);
    EXPECT_FLOAT_EQ(interp.elapsedTime, 0.0F);
    EXPECT_FLOAT_EQ(interp.velocityX, 0.0F);
    EXPECT_FLOAT_EQ(interp.velocityY, 0.0F);
    EXPECT_EQ(interp.mode, InterpolationMode::Linear);
    EXPECT_TRUE(interp.enabled);
}

TEST_F(InterpolationControllerTests, SetInterpolationTimeUpdatesTime)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    controller.setInterpolationTime(registry, entity, 0.5F);

    EXPECT_FLOAT_EQ(interp.interpolationTime, 0.5F);
}

TEST_F(InterpolationControllerTests, SetInterpolationTimeRejectsNegative)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.interpolationTime = 1.0F;

    controller.setInterpolationTime(registry, entity, -0.5F);

    EXPECT_FLOAT_EQ(interp.interpolationTime, 1.0F);
}

TEST_F(InterpolationControllerTests, SetInterpolationTimeRejectsZero)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.interpolationTime = 1.0F;

    controller.setInterpolationTime(registry, entity, 0.0F);

    EXPECT_FLOAT_EQ(interp.interpolationTime, 1.0F);
}

TEST_F(InterpolationControllerTests, IsAtTargetReturnsTrueWhenAtTarget)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);
    auto& transform = registry.emplace<TransformComponent>(entity);

    interp.targetX = 100.0F;
    interp.targetY = 200.0F;
    transform.x    = 100.0F;
    transform.y    = 200.0F;

    EXPECT_TRUE(controller.isAtTarget(registry, entity));
}

TEST_F(InterpolationControllerTests, IsAtTargetReturnsFalseWhenNotAtTarget)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);
    auto& transform = registry.emplace<TransformComponent>(entity);

    interp.targetX = 100.0F;
    interp.targetY = 200.0F;
    transform.x    = 50.0F;
    transform.y    = 100.0F;

    EXPECT_FALSE(controller.isAtTarget(registry, entity));
}

TEST_F(InterpolationControllerTests, IsAtTargetUsesThreshold)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);
    auto& transform = registry.emplace<TransformComponent>(entity);

    interp.targetX = 100.0F;
    interp.targetY = 200.0F;
    transform.x    = 100.005F;
    transform.y    = 200.005F;

    EXPECT_TRUE(controller.isAtTarget(registry, entity, 0.01F));
    EXPECT_FALSE(controller.isAtTarget(registry, entity, 0.001F));
}

TEST_F(InterpolationControllerTests, GetProgressReturnsZeroAtStart)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.elapsedTime       = 0.0F;
    interp.interpolationTime = 1.0F;

    EXPECT_FLOAT_EQ(controller.getProgress(registry, entity), 0.0F);
}

TEST_F(InterpolationControllerTests, GetProgressReturnsOneAtEnd)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.elapsedTime       = 1.0F;
    interp.interpolationTime = 1.0F;

    EXPECT_FLOAT_EQ(controller.getProgress(registry, entity), 1.0F);
}

TEST_F(InterpolationControllerTests, GetProgressReturnsMidpoint)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.elapsedTime       = 0.5F;
    interp.interpolationTime = 1.0F;

    EXPECT_FLOAT_EQ(controller.getProgress(registry, entity), 0.5F);
}

TEST_F(InterpolationControllerTests, GetProgressClampedToOne)
{
    EntityId entity = registry.createEntity();
    auto& interp    = registry.emplace<InterpolationComponent>(entity);

    interp.elapsedTime       = 2.0F;
    interp.interpolationTime = 1.0F;

    EXPECT_FLOAT_EQ(controller.getProgress(registry, entity), 1.0F);
}

TEST_F(InterpolationControllerTests, SetTargetOnDeadEntityNoThrow)
{
    EntityId entity = registry.createEntity();
    registry.emplace<InterpolationComponent>(entity);
    registry.destroyEntity(entity);

    EXPECT_NO_THROW(controller.setTarget(registry, entity, 100.0F, 200.0F));
}

TEST_F(InterpolationControllerTests, SetTargetWithoutComponentNoThrow)
{
    EntityId entity = registry.createEntity();

    EXPECT_NO_THROW(controller.setTarget(registry, entity, 100.0F, 200.0F));
}

TEST_F(InterpolationControllerTests, DisableWithoutTransformNoThrow)
{
    EntityId entity = registry.createEntity();
    registry.emplace<InterpolationComponent>(entity);

    EXPECT_NO_THROW(controller.disable(registry, entity));
}

TEST_F(InterpolationControllerTests, IsAtTargetReturnsFalseForDeadEntity)
{
    EntityId entity = registry.createEntity();
    registry.emplace<InterpolationComponent>(entity);
    registry.emplace<TransformComponent>(entity);
    registry.destroyEntity(entity);

    EXPECT_FALSE(controller.isAtTarget(registry, entity));
}

TEST_F(InterpolationControllerTests, GetProgressReturnsZeroForDeadEntity)
{
    EntityId entity = registry.createEntity();
    registry.emplace<InterpolationComponent>(entity);
    registry.destroyEntity(entity);

    EXPECT_FLOAT_EQ(controller.getProgress(registry, entity), 0.0F);
}
