#include "graphics/backends/sfml/SFMLMusic.hpp"

bool SFMLMusic::openFromFile(const std::string& filename) {
    return music_.openFromFile(filename);
}

void SFMLMusic::play() {
    music_.play();
}

void SFMLMusic::pause() {
    music_.pause();
}

void SFMLMusic::stop() {
    music_.stop();
}

IMusic::Status SFMLMusic::getStatus() const {
     sf::SoundSource::Status status = music_.getStatus();

     switch (status) {
        case sf::SoundSource::Status::Stopped: return IMusic::Status::Stopped;
        case sf::SoundSource::Status::Paused: return IMusic::Status::Paused;
        case sf::SoundSource::Status::Playing: return IMusic::Status::Playing;
        default: return IMusic::Status::Stopped;
    }
}

void SFMLMusic::setVolume(float volume) {
    music_.setVolume(volume);
}

float SFMLMusic::getVolume() const {
    return music_.getVolume();
}

void SFMLMusic::setLoop(bool loop) {
    music_.setLooping(loop);
}

bool SFMLMusic::getLoop() const {
    return music_.isLooping();
}
