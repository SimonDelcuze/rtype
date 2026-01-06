#pragma once

#include <cstdint>

struct Color
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;

    Color(std::uint8_t r = 0, std::uint8_t g = 0, std::uint8_t b = 0, std::uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color White;
    static const Color Black;
    static const Color Yellow;
    static const Color Magenta;
    static const Color Cyan;
    static const Color Transparent;

    bool operator==(const Color& other) const
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    bool operator!=(const Color& other) const
    {
        return !(*this == other);
    }
};

inline const Color Color::Red{255, 0, 0};
inline const Color Color::Green{0, 255, 0};
inline const Color Color::Blue{0, 0, 255};
inline const Color Color::White{255, 255, 255};
inline const Color Color::Black{0, 0, 0};
inline const Color Color::Yellow{255, 255, 0};
inline const Color Color::Magenta{255, 0, 255};
inline const Color Color::Cyan{0, 255, 255};
inline const Color Color::Transparent{0, 0, 0, 0};

template <typename T> struct Vector2
{
    T x{0};
    T y{0};

    bool operator==(const Vector2& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Vector2& other) const
    {
        return !(*this == other);
    }
};

using Vector2f = Vector2<float>;
using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned int>;

template <typename T> struct Rect
{
    T left{0};
    T top{0};
    T width{0};
    T height{0};

    constexpr Rect() = default;
    constexpr Rect(T l, T t, T w, T h) : left(l), top(t), width(w), height(h) {}

    bool operator==(const Rect& other) const
    {
        return left == other.left && top == other.top && width == other.width && height == other.height;
    }

    bool operator!=(const Rect& other) const
    {
        return !(*this == other);
    }
};

using FloatRect = Rect<float>;
using IntRect   = Rect<int>;
