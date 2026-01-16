#pragma once

#include "ecs/ResetValue.hpp"
#include "graphics/abstraction/Common.hpp"

struct BoxComponent
{
    float width            = 100.0F;
    float height           = 50.0F;
    Color fillColor        = Color{50, 50, 50};
    Color outlineColor     = Color{100, 100, 100};
    Color focusColor       = Color{100, 200, 255};
    float outlineThickness = 2.0F;
    bool visible           = true;

    static BoxComponent create(float w, float h, Color fill, Color outline)
    {
        BoxComponent b;
        b.width        = w;
        b.height       = h;
        b.fillColor    = fill;
        b.outlineColor = outline;
        return b;
    }
};

template <> inline void resetValue<BoxComponent>(BoxComponent& value)
{
    value = BoxComponent{};
}
