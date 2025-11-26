#include "components/LivesComponent.hpp"

#include <gtest/gtest.h>

TEST(LivesComponent, CreateSetsCurrentAndMax)
{
    auto l = LivesComponent::create(3, 5);
    EXPECT_EQ(l.current, 3);
    EXPECT_EQ(l.max, 5);
}

TEST(LivesComponent, DefaultIsZeroAndDead)
{
    LivesComponent l{};
    EXPECT_EQ(l.current, 0);
    EXPECT_EQ(l.max, 0);
    EXPECT_TRUE(l.isDead());
}

TEST(LivesComponent, LoseLifeDecrements)
{
    auto l = LivesComponent::create(3, 3);
    l.loseLife();
    EXPECT_EQ(l.current, 2);
    EXPECT_FALSE(l.isDead());
}

TEST(LivesComponent, LoseLifeClampToZero)
{
    auto l = LivesComponent::create(1, 1);
    l.loseLife(5);
    EXPECT_EQ(l.current, 0);
    EXPECT_TRUE(l.isDead());
}

TEST(LivesComponent, MultipleLossesAccumulate)
{
    auto l = LivesComponent::create(5, 5);
    l.loseLife(2);
    l.loseLife(2);
    EXPECT_EQ(l.current, 1);
}

TEST(LivesComponent, LoseLifeIgnoresNonPositive)
{
    auto l = LivesComponent::create(4, 4);
    l.loseLife(0);
    l.loseLife(-2);
    EXPECT_EQ(l.current, 4);
}

TEST(LivesComponent, AddLifeIncreasesUpToMax)
{
    auto l = LivesComponent::create(1, 3);
    l.addLife();
    EXPECT_EQ(l.current, 2);
    l.addLife(5);
    EXPECT_EQ(l.current, 3);
}

TEST(LivesComponent, AddLifeFromZeroRevives)
{
    auto l = LivesComponent::create(0, 2);
    l.addLife();
    EXPECT_EQ(l.current, 1);
    EXPECT_FALSE(l.isDead());
}

TEST(LivesComponent, AddLifeIgnoresNonPositive)
{
    auto l = LivesComponent::create(1, 2);
    l.addLife(0);
    l.addLife(-3);
    EXPECT_EQ(l.current, 1);
}

TEST(LivesComponent, AddExtraLifeIncreasesMaxAndCurrent)
{
    auto l = LivesComponent::create(2, 3);
    l.addExtraLife();
    EXPECT_EQ(l.max, 4);
    EXPECT_EQ(l.current, 3);
}

TEST(LivesComponent, AddExtraLifeMultiple)
{
    auto l = LivesComponent::create(1, 1);
    l.addExtraLife(3);
    EXPECT_EQ(l.max, 4);
    EXPECT_EQ(l.current, 4);
}

TEST(LivesComponent, AddExtraLifeIgnoresNonPositive)
{
    auto l = LivesComponent::create(2, 2);
    l.addExtraLife(0);
    l.addExtraLife(-5);
    EXPECT_EQ(l.max, 2);
    EXPECT_EQ(l.current, 2);
}

TEST(LivesComponent, SetMaxReducesCurrentIfAbove)
{
    auto l = LivesComponent::create(5, 5);
    l.setMax(3);
    EXPECT_EQ(l.max, 3);
    EXPECT_EQ(l.current, 3);
}

TEST(LivesComponent, SetMaxToZeroMakesDead)
{
    auto l = LivesComponent::create(2, 2);
    l.setMax(0);
    EXPECT_EQ(l.max, 0);
    EXPECT_EQ(l.current, 0);
    EXPECT_TRUE(l.isDead());
}

TEST(LivesComponent, ResetToMaxRestoresFull)
{
    auto l = LivesComponent::create(5, 5);
    l.loseLife(3);
    l.resetToMax();
    EXPECT_EQ(l.current, 5);
    EXPECT_FALSE(l.isDead());
}
