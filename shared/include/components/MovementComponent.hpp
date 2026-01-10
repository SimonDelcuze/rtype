#pragma once

#include <cstdint>

enum class MovementPattern : std::uint8_t
{
    Linear,
    Zigzag,
    Sine,
    FollowPlayer
};

struct MovementComponent
{
    MovementPattern pattern = MovementPattern::Linear;
    float speed             = 0.0F;
    float amplitude         = 0.0F;
    float frequency         = 0.0F;
    float phase             = 0.0F;
    float time              = 0.0F;

    static MovementComponent linear(float speed);
    static MovementComponent zigzag(float speed, float amplitude, float frequency);
    static MovementComponent sine(float speed, float amplitude, float frequency, float phase = 0.0F);
    static MovementComponent followPlayer(float speed);
};

inline MovementComponent MovementComponent::linear(float speed)
{
    MovementComponent component;
    component.pattern   = MovementPattern::Linear;
    component.speed     = speed;
    component.phase     = 0.0F;
    component.frequency = 0.0F;
    component.amplitude = 0.0F;
    component.time      = 0.0F;
    return component;
}

inline MovementComponent MovementComponent::zigzag(float speed, float amplitude, float frequency)
{
    MovementComponent component;
    component.pattern   = MovementPattern::Zigzag;
    component.speed     = speed;
    component.amplitude = amplitude;
    component.frequency = frequency;
    component.phase     = 0.0F;
    component.time      = 0.0F;
    return component;
}

inline MovementComponent MovementComponent::sine(float speed, float amplitude, float frequency, float phase)
{
    MovementComponent component;
    component.pattern   = MovementPattern::Sine;
    component.speed     = speed;
    component.amplitude = amplitude;
    component.frequency = frequency;
    component.phase     = phase;
    component.time      = 0.0F;
    return component;
}

inline MovementComponent MovementComponent::followPlayer(float speed)
{
    MovementComponent component;
    component.pattern   = MovementPattern::FollowPlayer;
    component.speed     = speed;
    component.phase     = 0.0F;
    component.frequency = 0.0F;
    component.amplitude = 0.0F;
    component.time      = 0.0F;
    return component;
}
