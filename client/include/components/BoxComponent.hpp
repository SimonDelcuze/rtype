#pragma once

#include "ecs/ResetValue.hpp"

#include <SFML/Graphics/Color.hpp>

struct BoxComponent
{
    float width            = 100.0F;
    float height           = 50.0F;
    sf::Color fillColor    = sf::Color(50, 50, 50);
    sf::Color outlineColor = sf::Color(100, 100, 100);
    sf::Color focusColor   = sf::Color(100, 200, 255);
    float outlineThickness = 2.0F;

    static BoxComponent create(float w, float h, sf::Color fill, sf::Color outline)
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
