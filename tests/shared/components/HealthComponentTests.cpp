#include "components/HealthComponent.hpp"

#include <gtest/gtest.h>

TEST(HealthComponent, Create)
{
    auto h = HealthComponent::create(150);

    EXPECT_EQ(h.current, 150);
    EXPECT_EQ(h.max, 150);
}

TEST(HealthComponent, Damage)
{
    auto h = HealthComponent::create(100);
    h.damage(30);

    EXPECT_EQ(h.current, 70);
    EXPECT_FALSE(h.isDead());
}

TEST(HealthComponent, DamageClampToZero)
{
    auto h = HealthComponent::create(50);
    h.damage(100);

    EXPECT_EQ(h.current, 0);
    EXPECT_TRUE(h.isDead());
}

TEST(HealthComponent, Heal)
{
    auto h = HealthComponent::create(100);
    h.damage(50);
    h.heal(30);

    EXPECT_EQ(h.current, 80);
}

TEST(HealthComponent, HealClampToMax)
{
    auto h = HealthComponent::create(100);
    h.damage(10);
    h.heal(50);

    EXPECT_EQ(h.current, 100);
}

TEST(HealthComponent, Percentage)
{
    auto h = HealthComponent::create(100);
    h.damage(25);

    EXPECT_FLOAT_EQ(h.getPercentage(), 0.75F);
}
