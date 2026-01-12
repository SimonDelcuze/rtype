#pragma once

#include "ecs/ResetValue.hpp"

#include <functional>
#include <string>

struct InputFieldComponent
{
    std::string value;
    std::string placeholder;
    std::size_t maxLength               = 32;
    bool focused                        = false;
    bool passwordField                  = false;
    std::function<bool(char)> validator = nullptr;
    float paddingX                      = 10.0F;
    float paddingY                      = 13.0F;
    bool centerVertically               = false;
    bool editable                       = true;

    static InputFieldComponent create(const std::string& defaultValue, std::size_t maxLen)
    {
        InputFieldComponent c;
        c.value     = defaultValue;
        c.maxLength = maxLen;
        return c;
    }

    static InputFieldComponent password(const std::string& defaultValue, std::size_t maxLen)
    {
        InputFieldComponent c;
        c.value         = defaultValue;
        c.maxLength     = maxLen;
        c.passwordField = true;
        return c;
    }

    static InputFieldComponent ipField(const std::string& defaultValue = "127.0.0.1")
    {
        InputFieldComponent c;
        c.value     = defaultValue;
        c.maxLength = 15;
        c.validator = [](char ch) { return ch == '.' || (ch >= '0' && ch <= '9'); };
        return c;
    }

    static InputFieldComponent portField(const std::string& defaultValue = "50010")
    {
        InputFieldComponent c;
        c.value     = defaultValue;
        c.maxLength = 5;
        c.validator = [](char ch) { return ch >= '0' && ch <= '9'; };
        return c;
    }
};

template <> inline void resetValue<InputFieldComponent>(InputFieldComponent& value)
{
    value = InputFieldComponent{};
}
