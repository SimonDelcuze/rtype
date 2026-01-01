#pragma once

#include "Inputs.hpp"

enum class EventType
{
    Closed,
    Resized,
    LostFocus,
    GainedFocus,
    TextEntered,
    KeyPressed,
    KeyReleased,
    MouseWheelScrolled,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseEntered,
    MouseLeft,
    Count
};

struct SizeEvent
{
    unsigned int width;
    unsigned int height;
};

struct KeyEvent
{
    KeyCode code;
    bool alt;
    bool control;
    bool shift;
    bool system;
};

struct TextEvent
{
    unsigned int unicode;
};

struct MouseMoveEvent
{
    int x;
    int y;
};

struct MouseButtonEvent
{
    MouseButton button;
    int x;
    int y;
};

struct MouseWheelScrollEvent
{
    float delta;
    int x;
    int y;
};

struct Event
{
    EventType type;

    union {
        SizeEvent size;
        KeyEvent key;
        TextEvent text;
        MouseMoveEvent mouseMove;
        MouseButtonEvent mouseButton;
        MouseWheelScrollEvent mouseWheelScroll;
    };
};
