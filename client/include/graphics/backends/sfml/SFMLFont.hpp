#pragma once

#include "../../abstraction/IFont.hpp"
#include <SFML/Graphics/Font.hpp>

class SFMLFont : public IFont {
public:
    bool loadFromFile(const std::string& filename) override;
    const sf::Font& getSFMLFont() const;

private:
    sf::Font font_;
};
