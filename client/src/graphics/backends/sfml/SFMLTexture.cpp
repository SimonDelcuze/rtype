#include "graphics/backends/sfml/SFMLTexture.hpp"

#include "graphics/backends/sfml/SFMLCommon.hpp"

bool SFMLTexture::loadFromFile(const std::string& filename)
{
    return texture_.loadFromFile(filename);
}

bool SFMLTexture::create(unsigned int width, unsigned int height)
{
    return texture_.resize({width, height});
}

Vector2u SFMLTexture::getSize() const
{
    auto sz = texture_.getSize();
    return {sz.x, sz.y};
}

void SFMLTexture::setRepeated(bool repeated)
{
    texture_.setRepeated(repeated);
}

bool SFMLTexture::isRepeated() const
{
    return texture_.isRepeated();
}

void SFMLTexture::setSmooth(bool smooth)
{
    texture_.setSmooth(smooth);
}

bool SFMLTexture::isSmooth() const
{
    return texture_.isSmooth();
}

const sf::Texture& SFMLTexture::getSFMLTexture() const
{
    return texture_;
}
