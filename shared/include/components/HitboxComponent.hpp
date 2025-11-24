#pragma once

#include <cstdint>

struct HitboxComponent
{
    float width   = 0.0F;
    float height  = 0.0F;
    float offsetX = 0.0F;
    float offsetY = 0.0F;
    bool isActive = true;

    static HitboxComponent create(float width, float height, float offsetX = 0.0F, float offsetY = 0.0F);
    bool contains(float px, float py, float entityX, float entityY) const;
    bool intersects(const HitboxComponent& other, float x1, float y1, float x2, float y2) const;
};

inline HitboxComponent HitboxComponent::create(float width, float height, float offsetX, float offsetY)
{
    HitboxComponent h;
    h.width   = width;
    h.height  = height;
    h.offsetX = offsetX;
    h.offsetY = offsetY;
    return h;
}

inline bool HitboxComponent::contains(float px, float py, float entityX, float entityY) const
{
    float left   = entityX + offsetX;
    float top    = entityY + offsetY;
    float right  = left + width;
    float bottom = top + height;
    return px >= left && px <= right && py >= top && py <= bottom;
}

inline bool HitboxComponent::intersects(const HitboxComponent& other, float x1, float y1, float x2, float y2) const
{
    if (!isActive || !other.isActive) {
        return false;
    }
    float left1   = x1 + offsetX;
    float top1    = y1 + offsetY;
    float right1  = left1 + width;
    float bottom1 = top1 + height;

    float left2   = x2 + other.offsetX;
    float top2    = y2 + other.offsetY;
    float right2  = left2 + other.width;
    float bottom2 = top2 + other.height;

    return !(left1 >= right2 || right1 <= left2 || top1 >= bottom2 || bottom1 <= top2);
}
