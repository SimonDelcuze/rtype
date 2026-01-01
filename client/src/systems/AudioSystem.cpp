#include "systems/AudioSystem.hpp"

AudioSystem::AudioSystem(SoundManager& soundManager, GraphicsFactory& graphicsFactory)
    : soundManager_(soundManager), graphicsFactory_(graphicsFactory)
{}

void AudioSystem::update(Registry& registry, float /*deltaTime*/)
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
                auto bufferPtr = soundManager_.get(audio.soundId);
                if (bufferPtr) {
                    if (!soundPtr) {
                        soundPtr = graphicsFactory_.createSound();
                    }
                    if (soundPtr) {
                        soundPtr->setBuffer(*bufferPtr);
                        soundPtr->setVolume(audio.volume);
                        soundPtr->setPitch(audio.pitch);
                        soundPtr->setLoop(audio.loop);
                        soundPtr->play();
                        audio.isPlaying = true;
                    }
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
        EntityId entity = it->first;
        auto& soundPtr  = it->second;

        if (!registry.isAlive(entity) || !registry.has<AudioComponent>(entity)) {
            it = sounds_.erase(it);
            continue;
        }

        if (soundPtr && soundPtr->getStatus() == ISound::Stopped) {
            AudioComponent& audio = registry.get<AudioComponent>(entity);
            audio.isPlaying       = false;
        }
        ++it;
    }
}
