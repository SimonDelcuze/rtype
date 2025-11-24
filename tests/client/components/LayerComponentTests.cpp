#include "components/LayerComponent.hpp"

#include <gtest/gtest.h>

TEST(LayerComponent, StoresLayerValue)
{
    LayerComponent layer;
    EXPECT_EQ(layer.layer, 0);

    auto created = LayerComponent::create(5);
    EXPECT_EQ(created.layer, 5);
}
