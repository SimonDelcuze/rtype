#include "systems/AudioSystem.hpp"

AudioSystem::AudioSystem(SoundManager& soundManager) : soundManager_(soundManager) {}

void AudioSystem::update(Registry& registry)
{
    for (EntityId entity = 0; entity < registry.entityCount(); ++entity) {
        if (!registry.isAlive(entity) || !registry.has<AudioComponent>(entity)) {
            continue;
        }

        AudioComponent& audio = registry.get<AudioComponent>(entity);

        if (audio.action == AudioAction::None) {
            continue;
        }

        auto& soundPtr = sounds_[entity];

        switch (audio.action) {
            case AudioAction::Play: {
                const sf::SoundBuffer* buffer = soundManager_.get(audio.soundId);
                if (buffer != nullptr) {
                    soundPtr = std::make_unique<sf::Sound>(*buffer);
                    soundPtr->setVolume(audio.volume);
                    soundPtr->setPitch(audio.pitch);
                    soundPtr->setLooping(audio.loop);
                    soundPtr->play();
                    audio.isPlaying = true;
                }
                break;
            }
            case AudioAction::Stop:
                if (soundPtr) {
                    soundPtr->stop();
                }
                audio.isPlaying = false;
                break;
            case AudioAction::Pause:
                if (soundPtr) {
                    soundPtr->pause();
                }
                audio.isPlaying = false;
                break;
            default:
                break;
        }

        audio.action = AudioAction::None;
    }

    for (auto it = sounds_.begin(); it != sounds_.end();) {
        EntityId entity   = it->first;
        auto&    soundPtr = it->second;

        if (!registry.isAlive(entity) || !registry.has<AudioComponent>(entity)) {
            it = sounds_.erase(it);
            continue;
        }

        if (soundPtr && soundPtr->getStatus() == sf::Sound::Status::Stopped) {
            AudioComponent& audio = registry.get<AudioComponent>(entity);
            audio.isPlaying       = false;
        }
        ++it;
    }
}
