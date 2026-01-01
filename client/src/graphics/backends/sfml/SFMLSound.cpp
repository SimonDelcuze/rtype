#include "graphics/backends/sfml/SFMLSound.hpp"
#include "graphics/backends/sfml/SFMLSoundBuffer.hpp"

static const sf::SoundBuffer& getDummyBuffer() {
    static const sf::SoundBuffer buffer;
    return buffer;
}

SFMLSound::SFMLSound() : sound_(getDummyBuffer()) {
}

void SFMLSound::setBuffer(const ISoundBuffer& buffer) {
    const auto* sfmlBuffer = dynamic_cast<const SFMLSoundBuffer*>(&buffer);
    if (sfmlBuffer) {
        sound_.setBuffer(sfmlBuffer->getSFMLSoundBuffer());
    }
}

void SFMLSound::play() {
    sound_.play();
}

void SFMLSound::pause() {
    sound_.pause();
}

void SFMLSound::stop() {
    sound_.stop();
}

ISound::Status SFMLSound::getStatus() const {
    sf::SoundSource::Status status = sound_.getStatus();
    switch (status) {
        case sf::SoundSource::Status::Stopped: return ISound::Status::Stopped;
        case sf::SoundSource::Status::Paused: return ISound::Status::Paused;
        case sf::SoundSource::Status::Playing: return ISound::Status::Playing;
        default: return ISound::Status::Stopped;
    }
}

void SFMLSound::setVolume(float volume) {
    sound_.setVolume(volume);
}

float SFMLSound::getVolume() const {
    return sound_.getVolume();
}

void SFMLSound::setPitch(float pitch) {
    sound_.setPitch(pitch);
}

float SFMLSound::getPitch() const {
    return sound_.getPitch();
}

void SFMLSound::setLoop(bool loop) {
    sound_.setLooping(loop);
}

bool SFMLSound::getLoop() const {
    return sound_.isLooping();
}
