#pragma once
#include "../../abstraction/ISoundBuffer.hpp"

#include <SFML/Audio/SoundBuffer.hpp>

class SFMLSoundBuffer : public ISoundBuffer
{
  public:
    bool loadFromFile(const std::string& filename) override;
    const sf::SoundBuffer& getSFMLSoundBuffer() const;

  private:
    sf::SoundBuffer buffer_;
};
