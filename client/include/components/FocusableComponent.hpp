#pragma once

#include "ecs/ResetValue.hpp"

struct FocusableComponent
{
    int tabOrder = 0;
    bool focused = false;

    static FocusableComponent create(int order)
    {
        FocusableComponent f;
        f.tabOrder = order;
        return f;
    }
};

template <> inline void resetValue<FocusableComponent>(FocusableComponent& value)
{
    value = FocusableComponent{};
}
