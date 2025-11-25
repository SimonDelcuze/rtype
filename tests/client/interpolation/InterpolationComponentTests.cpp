#include "components/InterpolationComponent.hpp"

#include <gtest/gtest.h>

TEST(InterpolationComponentTests, DefaultValues)
{
    InterpolationComponent interp;

    EXPECT_FLOAT_EQ(interp.previousX, 0.0F);
    EXPECT_FLOAT_EQ(interp.previousY, 0.0F);
    EXPECT_FLOAT_EQ(interp.targetX, 0.0F);
    EXPECT_FLOAT_EQ(interp.targetY, 0.0F);
    EXPECT_FLOAT_EQ(interp.elapsedTime, 0.0F);
    EXPECT_FLOAT_EQ(interp.interpolationTime, 0.1F);
    EXPECT_EQ(interp.mode, InterpolationMode::Linear);
    EXPECT_TRUE(interp.enabled);
}

TEST(InterpolationComponentTests, SetTargetUpdatesValues)
{
    InterpolationComponent interp;
    interp.targetX = 10.0F;
    interp.targetY = 20.0F;

    interp.setTarget(30.0F, 40.0F);

    EXPECT_FLOAT_EQ(interp.previousX, 10.0F);
    EXPECT_FLOAT_EQ(interp.previousY, 20.0F);
    EXPECT_FLOAT_EQ(interp.targetX, 30.0F);
    EXPECT_FLOAT_EQ(interp.targetY, 40.0F);
    EXPECT_FLOAT_EQ(interp.elapsedTime, 0.0F);
}

TEST(InterpolationComponentTests, SetTargetWithVelocity)
{
    InterpolationComponent interp;
    interp.setTargetWithVelocity(100.0F, 200.0F, 5.0F, -3.0F);

    EXPECT_FLOAT_EQ(interp.targetX, 100.0F);
    EXPECT_FLOAT_EQ(interp.targetY, 200.0F);
    EXPECT_FLOAT_EQ(interp.velocityX, 5.0F);
    EXPECT_FLOAT_EQ(interp.velocityY, -3.0F);
    EXPECT_FLOAT_EQ(interp.elapsedTime, 0.0F);
}

TEST(InterpolationComponentTests, MultipleSetTargetCalls)
{
    InterpolationComponent interp;

    interp.setTarget(10.0F, 20.0F);
    EXPECT_FLOAT_EQ(interp.targetX, 10.0F);

    interp.setTarget(30.0F, 40.0F);
    EXPECT_FLOAT_EQ(interp.previousX, 10.0F);
    EXPECT_FLOAT_EQ(interp.targetX, 30.0F);

    interp.setTarget(50.0F, 60.0F);
    EXPECT_FLOAT_EQ(interp.previousX, 30.0F);
    EXPECT_FLOAT_EQ(interp.targetX, 50.0F);
}

TEST(InterpolationComponentTests, InterpolationModeCanBeChanged)
{
    InterpolationComponent interp;

    interp.mode = InterpolationMode::None;
    EXPECT_EQ(interp.mode, InterpolationMode::None);

    interp.mode = InterpolationMode::Extrapolate;
    EXPECT_EQ(interp.mode, InterpolationMode::Extrapolate);

    interp.mode = InterpolationMode::Linear;
    EXPECT_EQ(interp.mode, InterpolationMode::Linear);
}

TEST(InterpolationComponentTests, EnabledCanBeToggled)
{
    InterpolationComponent interp;

    EXPECT_TRUE(interp.enabled);

    interp.enabled = false;
    EXPECT_FALSE(interp.enabled);

    interp.enabled = true;
    EXPECT_TRUE(interp.enabled);
}

TEST(InterpolationComponentTests, CustomInterpolationTime)
{
    InterpolationComponent interp;
    interp.interpolationTime = 0.05F;

    EXPECT_FLOAT_EQ(interp.interpolationTime, 0.05F);
}
