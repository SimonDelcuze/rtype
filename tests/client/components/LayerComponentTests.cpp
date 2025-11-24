#include "components/LayerComponent.hpp"

#include <gtest/gtest.h>

TEST(LayerComponent, DefaultLayerIsZero)
{
    LayerComponent layer;
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
