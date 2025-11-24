#include "components/VelocityComponent.hpp"

#include <gtest/gtest.h>

TEST(VelocityComponent, Create)
{
    auto v = VelocityComponent::create(100.0F, -50.0F);

    EXPECT_FLOAT_EQ(v.vx, 100.0F);
    EXPECT_FLOAT_EQ(v.vy, -50.0F);
}
