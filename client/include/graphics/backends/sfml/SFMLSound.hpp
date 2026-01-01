#pragma once
#include "../../abstraction/ISound.hpp"

#include <SFML/Audio/Sound.hpp>

class SFMLSound : public ISound
{
  public:
    SFMLSound();
    void setBuffer(const ISoundBuffer& buffer) override;
    void play() override;
    void pause() override;
    void stop() override;
    Status getStatus() const override;

    void setVolume(float volume) override;
    float getVolume() const override;
    void setPitch(float pitch) override;
    float getPitch() const override;
    void setLoop(bool loop) override;
    bool getLoop() const override;

  private:
    sf::Sound sound_;
};
