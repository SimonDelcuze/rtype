#pragma once

struct LayerComponent
{
    int layer = 0;

    static LayerComponent create(int layer)
    {
        LayerComponent l;
        l.layer = layer;
        return l;
    }
};
