#pragma once

#include "config/RenderLayers.hpp"

struct LayerComponent
{
    int layer = RenderLayer::Entities;

    static LayerComponent create(int layer)
    {
        LayerComponent l;
        l.layer = layer;
        return l;
    }
};
