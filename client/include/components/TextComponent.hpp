#pragma once

#include "ecs/ResetValue.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <optional>
#include <string>

struct TextComponent
{
    std::string fontId;
    unsigned int characterSize = 24;
    sf::Color color            = sf::Color::White;
    std::string content;
    std::optional<sf::Text> text;

    static TextComponent create(const std::string& font, unsigned int size, const sf::Color& c)
    {
        TextComponent t;
        t.fontId        = font;
        t.characterSize = size;
        t.color         = c;
        return t;
    }
};

template <> inline void resetValue<TextComponent>(TextComponent& value)
{
    value = TextComponent{};
}
