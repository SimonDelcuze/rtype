#include "components/LayerComponent.hpp"

#include <gtest/gtest.h>

TEST(LayerComponent, DefaultLayerIsEntities)
{
    LayerComponent layer;
    EXPECT_EQ(layer.layer, RenderLayer::Entities);
    EXPECT_EQ(layer.layer, 0);
}

TEST(LayerComponent, CreateSetsProvidedValue)
{
    auto created = LayerComponent::create(5);
    EXPECT_EQ(created.layer, 5);
}

TEST(LayerComponent, SupportsNegativeLayer)
{
    auto created = LayerComponent::create(-3);
    EXPECT_EQ(created.layer, -3);
}

TEST(LayerComponent, CopyAndAssignPreserveValue)
{
    auto original = LayerComponent::create(2);
    LayerComponent copy(original);
    EXPECT_EQ(copy.layer, 2);

    LayerComponent assigned;
    assigned.layer = -7;
    assigned       = original;
    EXPECT_EQ(assigned.layer, 2);
}

TEST(LayerComponent, MutableLayerCanBeUpdated)
{
    LayerComponent layer;
    layer.layer = 42;
    EXPECT_EQ(layer.layer, 42);
}

TEST(LayerComponent, InstancesAreIndependent)
{
    auto a  = LayerComponent::create(1);
    auto b  = LayerComponent::create(4);
    a.layer = 99;
    EXPECT_EQ(a.layer, 99);
    EXPECT_EQ(b.layer, 4);
}

TEST(LayerComponent, RenderLayerConstants)
{
    EXPECT_LT(RenderLayer::Background, RenderLayer::Entities);
    EXPECT_LT(RenderLayer::Midground, RenderLayer::Entities);
    EXPECT_LT(RenderLayer::Entities, RenderLayer::Effects);
    EXPECT_LT(RenderLayer::Effects, RenderLayer::UI);
    EXPECT_LT(RenderLayer::UI, RenderLayer::HUD);
    EXPECT_LT(RenderLayer::HUD, RenderLayer::Debug);
}

TEST(LayerComponent, CreateWithRenderLayerConstants)
{
    auto background = LayerComponent::create(RenderLayer::Background);
    auto entities   = LayerComponent::create(RenderLayer::Entities);
    auto hud        = LayerComponent::create(RenderLayer::HUD);

    EXPECT_EQ(background.layer, -100);
    EXPECT_EQ(entities.layer, 0);
    EXPECT_EQ(hud.layer, 150);
}
