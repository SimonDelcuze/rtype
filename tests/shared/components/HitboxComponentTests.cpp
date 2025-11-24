#include "components/HitboxComponent.hpp"

#include <gtest/gtest.h>

TEST(HitboxComponent, Create)
{
    auto h = HitboxComponent::create(32.0F, 64.0F, 5.0F, 10.0F);

    EXPECT_FLOAT_EQ(h.width, 32.0F);
    EXPECT_FLOAT_EQ(h.height, 64.0F);
    EXPECT_FLOAT_EQ(h.offsetX, 5.0F);
    EXPECT_FLOAT_EQ(h.offsetY, 10.0F);
    EXPECT_TRUE(h.isActive);
}

TEST(HitboxComponent, Contains)
{
    auto h = HitboxComponent::create(10.0F, 10.0F);

    EXPECT_TRUE(h.contains(5.0F, 5.0F, 0.0F, 0.0F));
    EXPECT_FALSE(h.contains(15.0F, 5.0F, 0.0F, 0.0F));
}

TEST(HitboxComponent, Intersects)
{
    auto h1 = HitboxComponent::create(10.0F, 10.0F);
    auto h2 = HitboxComponent::create(10.0F, 10.0F);

    EXPECT_TRUE(h1.intersects(h2, 0.0F, 0.0F, 5.0F, 5.0F));
    EXPECT_FALSE(h1.intersects(h2, 0.0F, 0.0F, 20.0F, 20.0F));
}

TEST(HitboxComponent, IntersectsInactive)
{
    auto h1     = HitboxComponent::create(10.0F, 10.0F);
    auto h2     = HitboxComponent::create(10.0F, 10.0F);
    h2.isActive = false;

    EXPECT_FALSE(h1.intersects(h2, 0.0F, 0.0F, 5.0F, 5.0F));
}
