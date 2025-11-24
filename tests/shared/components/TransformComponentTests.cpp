#include "components/TransformComponent.hpp"

#include <gtest/gtest.h>

TEST(TransformComponent, Create)
{
    auto t = TransformComponent::create(10.0F, 20.0F, 45.0F);

    EXPECT_FLOAT_EQ(t.x, 10.0F);
    EXPECT_FLOAT_EQ(t.y, 20.0F);
    EXPECT_FLOAT_EQ(t.rotation, 45.0F);
    EXPECT_FLOAT_EQ(t.scaleX, 1.0F);
    EXPECT_FLOAT_EQ(t.scaleY, 1.0F);
}

TEST(TransformComponent, Translate)
{
    auto t = TransformComponent::create(0.0F, 0.0F);
    t.translate(5.0F, -3.0F);

    EXPECT_FLOAT_EQ(t.x, 5.0F);
    EXPECT_FLOAT_EQ(t.y, -3.0F);
}

TEST(TransformComponent, Rotate)
{
    auto t = TransformComponent::create(0.0F, 0.0F, 10.0F);
    t.rotate(20.0F);

    EXPECT_FLOAT_EQ(t.rotation, 30.0F);
}

TEST(TransformComponent, Scale)
{
    auto t = TransformComponent::create(0.0F, 0.0F);
    t.scale(2.0F, 3.0F);

    EXPECT_FLOAT_EQ(t.scaleX, 2.0F);
    EXPECT_FLOAT_EQ(t.scaleY, 3.0F);
}
