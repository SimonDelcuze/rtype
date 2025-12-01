#pragma once

#include <cstdint>
#include <string>

using EntityId = std::uint32_t;

struct PlaySoundEvent
{
    std::string soundName;
    float volume = 1.0F;
    float pitch  = 1.0F;
    bool loop    = false;
};

struct PlayMusicEvent
{
    std::string musicName;
    float volume     = 1.0F;
    bool loop        = true;
    float fadeInTime = 0.0F;
};

struct StopMusicEvent
{
    float fadeOutTime = 0.0F;
};

struct StopSoundEvent
{
    std::string soundName;
};

struct SetMasterVolumeEvent
{
    float volume;
};

struct SetMusicVolumeEvent
{
    float volume;
};

struct SetSFXVolumeEvent
{
    float volume;
};

struct Play3DSoundEvent
{
    std::string soundName;
    float x;
    float y;
    float volume      = 1.0F;
    float minDistance = 100.0F;
    float attenuation = 1.0F;
};
