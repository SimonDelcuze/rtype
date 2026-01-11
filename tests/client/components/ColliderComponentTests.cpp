#include "components/ColliderComponent.hpp"

#include <gtest/gtest.h>

TEST(ColliderComponentTest, BoxFactory)
{
    auto c = ColliderComponent::box(100.0f, 50.0f, 10.0f, 20.0f, false);
    EXPECT_EQ(c.shape, ColliderComponent::Shape::Box);
    EXPECT_EQ(c.width, 100.0f);
    EXPECT_EQ(c.height, 50.0f);
    EXPECT_EQ(c.offsetX, 10.0f);
    EXPECT_EQ(c.offsetY, 20.0f);
    EXPECT_FALSE(c.isActive);
}

TEST(ColliderComponentTest, CircleFactory)
{
    auto c = ColliderComponent::circle(30.0f, 5.0f, 5.0f);
    EXPECT_EQ(c.shape, ColliderComponent::Shape::Circle);
    EXPECT_EQ(c.radius, 30.0f);
    EXPECT_EQ(c.offsetX, 5.0f);
    EXPECT_EQ(c.offsetY, 5.0f);
    EXPECT_TRUE(c.isActive);
}
