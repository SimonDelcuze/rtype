#include "components/ScoreComponent.hpp"

#include <gtest/gtest.h>

TEST(ScoreComponent, DefaultZero)
{
    ScoreComponent s{};
    EXPECT_EQ(s.value, 0);
    EXPECT_TRUE(s.isZero());
}

TEST(ScoreComponent, CreateSetsValue)
{
    auto s = ScoreComponent::create(500);
    EXPECT_EQ(s.value, 500);
    EXPECT_TRUE(s.isPositive());
}

TEST(ScoreComponent, CreateWithNegativeClamps)
{
    auto s = ScoreComponent::create(-10);
    EXPECT_EQ(s.value, -10);
}

TEST(ScoreComponent, AddPositiveIncrements)
{
    ScoreComponent s{};
    s.add(250);
    EXPECT_EQ(s.value, 250);
    EXPECT_TRUE(s.isPositive());
}

TEST(ScoreComponent, AddIgnoresZeroOrNegative)
{
    ScoreComponent s{};
    s.add(0);
    s.add(-50);
    EXPECT_EQ(s.value, 0);
}

TEST(ScoreComponent, AddMultipleAccumulated)
{
    ScoreComponent s{};
    s.add(100);
    s.add(200);
    EXPECT_EQ(s.value, 300);
}

TEST(ScoreComponent, SubtractReduces)
{
    auto s = ScoreComponent::create(500);
    s.subtract(120);
    EXPECT_EQ(s.value, 380);
}

TEST(ScoreComponent, SubtractClampsToZero)
{
    auto s = ScoreComponent::create(50);
    s.subtract(200);
    EXPECT_EQ(s.value, 0);
    EXPECT_TRUE(s.isZero());
}

TEST(ScoreComponent, SubtractIgnoresZeroOrNegative)
{
    auto s = ScoreComponent::create(100);
    s.subtract(0);
    s.subtract(-30);
    EXPECT_EQ(s.value, 100);
}

TEST(ScoreComponent, ResetClearsScore)
{
    auto s = ScoreComponent::create(999);
    s.reset();
    EXPECT_EQ(s.value, 0);
    EXPECT_TRUE(s.isZero());
}

TEST(ScoreComponent, SetClampsNegativeToZero)
{
    ScoreComponent s{};
    s.set(-5);
    EXPECT_EQ(s.value, 0);
}

TEST(ScoreComponent, SetAssignsPositive)
{
    ScoreComponent s{};
    s.set(1234);
    EXPECT_EQ(s.value, 1234);
}

TEST(ScoreComponent, PositiveCheck)
{
    auto s = ScoreComponent::create(1);
    EXPECT_TRUE(s.isPositive());
    s.subtract(1);
    EXPECT_FALSE(s.isPositive());
}

TEST(ScoreComponent, ChainOperations)
{
    ScoreComponent s{};
    s.add(100);
    s.subtract(20);
    s.add(50);
    EXPECT_EQ(s.value, 130);
}

TEST(ScoreComponent, LargeValues)
{
    ScoreComponent s{};
    s.add(1'000'000);
    EXPECT_EQ(s.value, 1'000'000);
}
