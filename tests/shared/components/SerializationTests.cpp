#include "components/Serialization.hpp"

#include <gtest/gtest.h>

TEST(Serialization, TransformRoundTrip)
{
    auto original   = TransformComponent::create(10.0F, 20.0F, 45.0F);
    original.scaleX = 2.0F;
    original.scaleY = 3.0F;

    std::vector<std::uint8_t> buffer;
    Serialization::serialize(buffer, original);

    std::size_t offset = 0;
    auto deserialized  = Serialization::deserializeTransform(buffer.data(), offset);

    EXPECT_FLOAT_EQ(deserialized.x, original.x);
    EXPECT_FLOAT_EQ(deserialized.y, original.y);
    EXPECT_FLOAT_EQ(deserialized.rotation, original.rotation);
    EXPECT_FLOAT_EQ(deserialized.scaleX, original.scaleX);
    EXPECT_FLOAT_EQ(deserialized.scaleY, original.scaleY);
}

TEST(Serialization, VelocityRoundTrip)
{
    auto original = VelocityComponent::create(100.0F, -50.0F);

    std::vector<std::uint8_t> buffer;
    Serialization::serialize(buffer, original);

    std::size_t offset = 0;
    auto deserialized  = Serialization::deserializeVelocity(buffer.data(), offset);

    EXPECT_FLOAT_EQ(deserialized.vx, original.vx);
    EXPECT_FLOAT_EQ(deserialized.vy, original.vy);
}

TEST(Serialization, HealthRoundTrip)
{
    auto original = HealthComponent::create(150);
    original.damage(30);

    std::vector<std::uint8_t> buffer;
    Serialization::serialize(buffer, original);

    std::size_t offset = 0;
    auto deserialized  = Serialization::deserializeHealth(buffer.data(), offset);

    EXPECT_EQ(deserialized.current, original.current);
    EXPECT_EQ(deserialized.max, original.max);
}

TEST(Serialization, OwnershipRoundTrip)
{
    auto original = OwnershipComponent::create(42, 3);

    std::vector<std::uint8_t> buffer;
    Serialization::serialize(buffer, original);

    std::size_t offset = 0;
    auto deserialized  = Serialization::deserializeOwnership(buffer.data(), offset);

    EXPECT_EQ(deserialized.ownerId, original.ownerId);
    EXPECT_EQ(deserialized.team, original.team);
}

TEST(Serialization, TagRoundTrip)
{
    auto original = TagComponent::create(EntityTag::Player | EntityTag::Projectile);

    std::vector<std::uint8_t> buffer;
    Serialization::serialize(buffer, original);

    std::size_t offset = 0;
    auto deserialized  = Serialization::deserializeTag(buffer.data(), offset);

    EXPECT_TRUE(deserialized.hasTag(EntityTag::Player));
    EXPECT_TRUE(deserialized.hasTag(EntityTag::Projectile));
    EXPECT_FALSE(deserialized.hasTag(EntityTag::Enemy));
}

TEST(Serialization, HitboxRoundTrip)
{
    auto original = HitboxComponent::create(32.0F, 64.0F, 5.0F, 10.0F);

    std::vector<std::uint8_t> buffer;
    Serialization::serialize(buffer, original);

    std::size_t offset = 0;
    auto deserialized  = Serialization::deserializeHitbox(buffer.data(), offset);

    EXPECT_FLOAT_EQ(deserialized.width, original.width);
    EXPECT_FLOAT_EQ(deserialized.height, original.height);
    EXPECT_FLOAT_EQ(deserialized.offsetX, original.offsetX);
    EXPECT_FLOAT_EQ(deserialized.offsetY, original.offsetY);
    EXPECT_EQ(deserialized.isActive, original.isActive);
}
