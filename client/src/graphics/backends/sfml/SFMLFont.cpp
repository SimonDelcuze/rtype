#include "graphics/backends/sfml/SFMLFont.hpp"

bool SFMLFont::loadFromFile(const std::string& filename)
{
    return font_.openFromFile(filename);
}

const sf::Font& SFMLFont::getSFMLFont() const
{
    return font_;
}
