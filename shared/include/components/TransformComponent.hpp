#pragma once

#include <cstdint>

struct TransformComponent
{
    float x        = 0.0F;
    float y        = 0.0F;
    float rotation = 0.0F;
    float scaleX   = 1.0F;
    float scaleY   = 1.0F;

    static TransformComponent create(float x, float y, float rotation = 0.0F);
    void translate(float dx, float dy);
    void rotate(float angle);
    void scale(float sx, float sy);
};

inline TransformComponent TransformComponent::create(float x, float y, float rotation)
{
    TransformComponent t;
    t.x        = x;
    t.y        = y;
    t.rotation = rotation;
    return t;
}

inline void TransformComponent::translate(float dx, float dy)
{
    x += dx;
    y += dy;
}

inline void TransformComponent::rotate(float angle)
{
    rotation += angle;
}

inline void TransformComponent::scale(float sx, float sy)
{
    scaleX *= sx;
    scaleY *= sy;
}
