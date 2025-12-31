#pragma once

#include <cstdint>

struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};

    static const Color Black;
    static const Color White;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Magenta;
    static const Color Cyan;
    static const Color Transparent;
};

template <typename T>
struct Vector2 {
    T x{0};
    T y{0};
};

using Vector2f = Vector2<float>;
using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned int>;

template <typename T>
struct Rect {
    T left{0};
    T top{0};
    T width{0};
    T height{0};
};

using FloatRect = Rect<float>;
using IntRect = Rect<int>;
