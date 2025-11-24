#pragma once

struct VelocityComponent
{
    float vx = 0.0F;
    float vy = 0.0F;

    static VelocityComponent create(float vx, float vy);
};

inline VelocityComponent VelocityComponent::create(float vx, float vy)
{
    VelocityComponent v;
    v.vx = vx;
    v.vy = vy;
    return v;
}
