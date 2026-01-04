#pragma once

struct IntroCinematicConfig
{
    float forwardDistance = 140.0F;
    float forwardDuration = 0.9F;
    float backDuration    = 1.1F;
    float settleDuration  = 0.2F;

    constexpr float totalDuration() const
    {
        return forwardDuration + backDuration + settleDuration;
    }
};

constexpr IntroCinematicConfig kIntroCinematicConfig{};
