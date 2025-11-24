#pragma once

#include <cstdint>
#include <string>

enum class AudioAction : std::uint8_t
{
    None,
    Play,
    Stop,
    Pause
};

struct AudioComponent
{
    std::string soundId;
    AudioAction action = AudioAction::None;
    float volume       = 100.0F;
    float pitch        = 1.0F;
    bool loop          = false;
    bool isPlaying     = false;

    static AudioComponent create(const std::string& soundId);
    void play();
    void play(const std::string& id);
    void stop();
    void pause();
};

inline AudioComponent AudioComponent::create(const std::string& soundId)
{
    AudioComponent a;
    a.soundId = soundId;
    return a;
}

inline void AudioComponent::play()
{
    action = AudioAction::Play;
}

inline void AudioComponent::play(const std::string& id)
{
    soundId = id;
    action  = AudioAction::Play;
}

inline void AudioComponent::stop()
{
    action = AudioAction::Stop;
}

inline void AudioComponent::pause()
{
    action = AudioAction::Pause;
}
