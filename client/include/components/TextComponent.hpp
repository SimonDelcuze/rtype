#pragma once

#include "ecs/ResetValue.hpp"
#include "graphics/abstraction/Common.hpp"
#include "graphics/abstraction/IText.hpp"

#include <memory>
#include <optional>
#include <string>

struct TextComponent
{
    std::string fontId;
    unsigned int characterSize = 24;
    Color color                = Color::White;
    std::string content;
    bool centered = false;
    float centerOffsetX{0.0F};
    float centerOffsetY{0.0F};
    std::shared_ptr<IText> text;

    static TextComponent create(const std::string& font, unsigned int size, const Color& c)
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
