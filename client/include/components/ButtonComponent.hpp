#pragma once

#include "ecs/ResetValue.hpp"

#include <functional>
#include <string>

struct ButtonComponent
{
    std::string label;
    bool hovered                  = false;
    bool pressed                  = false;
    std::function<void()> onClick = nullptr;
    float textOffsetX             = 0.0F;
    float textOffsetY             = 0.0F;
    bool autoRepeat               = false;
    float repeatDelay             = 0.35F;
    float repeatInterval          = 0.08F;
    float holdTimer               = 0.0F;

    static ButtonComponent create(const std::string& text, std::function<void()> callback)
    {
        ButtonComponent b;
        b.label   = text;
        b.onClick = callback;
        return b;
    }
};

template <> inline void resetValue<ButtonComponent>(ButtonComponent& value)
{
    value = ButtonComponent{};
}
