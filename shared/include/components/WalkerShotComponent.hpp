#pragma once

#include <cstdint>

struct WalkerShotComponent
{
    std::uint32_t ownerId     = 0;
    float tickDuration        = 0.16F;
    float elapsed             = 0.0F;
    std::int32_t currentTick  = 0;
    std::int32_t ascentTicks  = 4;
    std::int32_t hoverTicks   = 3;
    std::int32_t descendTicks = 4;
    float anchorOffsetX       = 0.0F;
    float anchorOffsetY       = -10.0F;
    float apexOffset          = 200.0F;

    static WalkerShotComponent create(std::uint32_t ownerId, float tickDuration = 0.16F, float anchorOffsetY = -10.0F,
                                      float apexOffset = 200.0F, float anchorOffsetX = 0.0F);
};

inline WalkerShotComponent WalkerShotComponent::create(std::uint32_t owner, float tickDur, float anchorOffset,
                                                       float apex, float anchorOffsetX)
{
    WalkerShotComponent c{};
    c.ownerId       = owner;
    c.tickDuration  = tickDur;
    c.anchorOffsetY = anchorOffset;
    c.apexOffset    = apex;
    c.anchorOffsetX = anchorOffsetX;
    return c;
}
