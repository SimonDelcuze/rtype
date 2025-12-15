#pragma once

#include <string>

struct DirectionalAnimationComponent
{
    std::string spriteId;
    std::string idleLabel;
    std::string upLabel;
    std::string downLabel;
    float threshold = 60.0F;

    enum class Phase
    {
        Idle,
        UpIn,
        UpHold,
        UpOut,
        DownIn,
        DownHold,
        DownOut
    };
    Phase phase     = Phase::Idle;
    float phaseTime = -1.0F;
};
