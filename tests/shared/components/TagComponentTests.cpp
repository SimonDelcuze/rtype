#include "components/TagComponent.hpp"

#include <gtest/gtest.h>

TEST(TagComponent, Create)
{
    auto t = TagComponent::create(EntityTag::Player);

    EXPECT_TRUE(t.hasTag(EntityTag::Player));
    EXPECT_FALSE(t.hasTag(EntityTag::Enemy));
}

TEST(TagComponent, AddTag)
{
    auto t = TagComponent::create(EntityTag::Player);
    t.addTag(EntityTag::Projectile);

    EXPECT_TRUE(t.hasTag(EntityTag::Player));
    EXPECT_TRUE(t.hasTag(EntityTag::Projectile));
}

TEST(TagComponent, RemoveTag)
{
    auto t = TagComponent::create(EntityTag::Player | EntityTag::Projectile);
    t.removeTag(EntityTag::Player);

    EXPECT_FALSE(t.hasTag(EntityTag::Player));
    EXPECT_TRUE(t.hasTag(EntityTag::Projectile));
}
