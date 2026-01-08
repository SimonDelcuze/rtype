#pragma once

#include <cstdint>

enum class InterpolationMode : std::uint8_t
{
    None,
    Linear,
    Extrapolate
};

struct InterpolationComponent
{
    float previousX = 0.0F;
    float previousY = 0.0F;

    float targetX = 0.0F;
    float targetY = 0.0F;

    float elapsedTime       = 0.0F;
    float interpolationTime = 0.1F;

    InterpolationMode mode = InterpolationMode::Linear;
    bool enabled           = true;

    float velocityX = 0.0F;
    float velocityY = 0.0F;

    float maxExtrapolationTime = 0.2F;

    void setTarget(float x, float y);
    void setTargetWithVelocity(float x, float y, float vx, float vy);
};
