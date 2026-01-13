#pragma once

#include "graphics/abstraction/Inputs.hpp"

#include <map>

struct KeyBindings
{
    KeyCode up       = KeyCode::Up;
    KeyCode down     = KeyCode::Down;
    KeyCode left     = KeyCode::Left;
    KeyCode right    = KeyCode::Right;
    KeyCode fire     = KeyCode::Space;
    KeyCode interact = KeyCode::E;

    static KeyBindings defaults()
    {
        return {};
    }
};
