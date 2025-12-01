#pragma once

#include <cstdint>
#include <string>

using EntityId = std::uint32_t;

struct CameraShakeEvent
{
    float intensity;
    float duration;
    float frequency = 30.0F;
};

struct ScreenFlashEvent
{
    float red;
    float green;
    float blue;
    float alpha;
    float duration;
};

struct SpawnParticleEffectEvent
{
    std::string effectType;
    float x;
    float y;
    float velocityX = 0.0F;
    float velocityY = 0.0F;
    float scale     = 1.0F;
};

struct AttachEffectToEntityEvent
{
    EntityId entityId;
    std::string effectType;
    float offsetX = 0.0F;
    float offsetY = 0.0F;
};

struct SetEntityVisibilityEvent
{
    EntityId entityId;
    bool visible;
};

struct SetEntityOpacityEvent
{
    EntityId entityId;
    float opacity;
};

struct PlayAnimationEvent
{
    EntityId entityId;
    std::string animationName;
    bool loop = true;
};

struct TintEntityEvent
{
    EntityId entityId;
    float red;
    float green;
    float blue;
    float duration;
};

struct HighlightEntityEvent
{
    EntityId entityId;
    bool enabled;
    float red   = 1.0F;
    float green = 1.0F;
    float blue  = 1.0F;
};

struct SpawnDecalEvent
{
    std::string decalType;
    float x;
    float y;
    float rotation = 0.0F;
    float scale    = 1.0F;
    float lifetime = 5.0F;
};
