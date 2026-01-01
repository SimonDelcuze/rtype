#pragma once
#include "../../abstraction/IMusic.hpp"

#include <SFML/Audio/Music.hpp>

class SFMLMusic : public IMusic
{
  public:
    bool openFromFile(const std::string& filename) override;
    void play() override;
    void pause() override;
    void stop() override;
    Status getStatus() const override;

    void setVolume(float volume) override;
    float getVolume() const override;
    void setLoop(bool loop) override;
    bool getLoop() const override;

  private:
    sf::Music music_;
};
