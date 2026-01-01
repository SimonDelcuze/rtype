#include "graphics/backends/sfml/SFMLSoundBuffer.hpp"

bool SFMLSoundBuffer::loadFromFile(const std::string& filename)
{
    return buffer_.loadFromFile(filename);
}

const sf::SoundBuffer& SFMLSoundBuffer::getSFMLSoundBuffer() const
{
    return buffer_;
}
