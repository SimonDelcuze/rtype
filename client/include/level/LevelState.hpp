#pragma once

#include <cstdint>

struct LevelState
{
    enum class ShieldFeedback
    {
        None,
        AlreadyActive,
        PurchaseRequested
    };

    std::uint16_t levelId     = 0;
    std::uint32_t seed        = 0;
    bool active               = false;
    bool introCinematicActive = false;
    float introCinematicTime  = 0.0F;
    bool safeZoneActive       = false;
    ShieldFeedback shieldFeedback     = ShieldFeedback::None;
    float shieldFeedbackTimeRemaining = 0.0F;
};
