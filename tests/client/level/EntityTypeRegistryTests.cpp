#include "level/EntityTypeRegistry.hpp"

#include <gtest/gtest.h>

TEST(EntityTypeRegistry, RegisterAndRetrieve)
{
    EntityTypeRegistry registry;

    RenderTypeData data{};
    data.frameCount    = 8;
    data.frameDuration = 0.1F;
    data.layer         = 5;

    registry.registerType(42, data);

    EXPECT_TRUE(registry.has(42));
    EXPECT_EQ(registry.size(), 1u);

    const auto* retrieved = registry.get(42);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->frameCount, 8);
    EXPECT_FLOAT_EQ(retrieved->frameDuration, 0.1F);
    EXPECT_EQ(retrieved->layer, 5);
}

TEST(EntityTypeRegistry, ReturnsNullForUnknown)
{
    EntityTypeRegistry registry;

    EXPECT_FALSE(registry.has(999));
    EXPECT_EQ(registry.get(999), nullptr);
}

TEST(EntityTypeRegistry, ClearsAll)
{
    EntityTypeRegistry registry;

    RenderTypeData data{};
    registry.registerType(1, data);
    registry.registerType(2, data);
    EXPECT_EQ(registry.size(), 2u);

    registry.clear();
    EXPECT_EQ(registry.size(), 0u);
    EXPECT_FALSE(registry.has(1));
    EXPECT_FALSE(registry.has(2));
}

TEST(EntityTypeRegistry, OverwritesExisting)
{
    EntityTypeRegistry registry;

    RenderTypeData d1{};
    d1.layer = 1;
    registry.registerType(10, d1);

    RenderTypeData d2{};
    d2.layer = 99;
    registry.registerType(10, d2);

    EXPECT_EQ(registry.size(), 1u);
    const auto* retrieved = registry.get(10);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->layer, 99);
}

TEST(EntityTypeRegistry, MultipleTypes)
{
    EntityTypeRegistry registry;

    for (std::uint16_t i = 0; i < 100; ++i) {
        RenderTypeData data{};
        data.layer = static_cast<std::uint8_t>(i % 10);
        registry.registerType(i, data);
    }

    EXPECT_EQ(registry.size(), 100u);

    for (std::uint16_t i = 0; i < 100; ++i) {
        EXPECT_TRUE(registry.has(i));
        const auto* data = registry.get(i);
        ASSERT_NE(data, nullptr);
        EXPECT_EQ(data->layer, i % 10);
    }
}
