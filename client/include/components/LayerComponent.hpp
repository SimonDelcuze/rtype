#pragma once

namespace RenderLayer
{
    constexpr int Background = -100;
    constexpr int Midground  = -50;
    constexpr int Entities   = 0;
    constexpr int Effects    = 50;
    constexpr int UI         = 100;
    constexpr int HUD        = 150;
    constexpr int Debug      = 200;
}

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
