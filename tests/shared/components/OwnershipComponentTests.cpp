#include "components/OwnershipComponent.hpp"

#include <gtest/gtest.h>

TEST(OwnershipComponent, Create)
{
    auto o = OwnershipComponent::create(42, 1);

    EXPECT_EQ(o.ownerId, 42);
    EXPECT_EQ(o.team, 1);
}

TEST(OwnershipComponent, SameTeam)
{
    auto o1 = OwnershipComponent::create(1, 1);
    auto o2 = OwnershipComponent::create(2, 1);
    auto o3 = OwnershipComponent::create(3, 2);

    EXPECT_TRUE(o1.isSameTeam(o2));
    EXPECT_FALSE(o1.isSameTeam(o3));
}

TEST(OwnershipComponent, SameOwner)
{
    auto o1 = OwnershipComponent::create(1, 1);
    auto o2 = OwnershipComponent::create(1, 2);
    auto o3 = OwnershipComponent::create(2, 1);

    EXPECT_TRUE(o1.isSameOwner(o2));
    EXPECT_FALSE(o1.isSameOwner(o3));
}
