#include "graphics/backends/sfml/SFMLText.hpp"

#include "graphics/backends/sfml/SFMLCommon.hpp"
#include "graphics/backends/sfml/SFMLFont.hpp"

static const sf::Font& getDummyFont()
{
    static const sf::Font font;
    return font;
}

SFMLText::SFMLText() : text_(getDummyFont()) {}

void SFMLText::setFont(const IFont& font)
{
    const auto* sfmlFont = dynamic_cast<const SFMLFont*>(&font);
    if (sfmlFont) {
        text_.setFont(sfmlFont->getSFMLFont());
    }
}

void SFMLText::setString(const std::string& string)
{
    text_.setString(string);
}

void SFMLText::setCharacterSize(unsigned int size)
{
    text_.setCharacterSize(size);
}

void SFMLText::setFillColor(const Color& color)
{
    text_.setFillColor(toSFML(color));
}

void SFMLText::setOutlineColor(const Color& color)
{
    text_.setOutlineColor(toSFML(color));
}

void SFMLText::setOutlineThickness(float thickness)
{
    text_.setOutlineThickness(thickness);
}

void SFMLText::setPosition(const Vector2f& position)
{
    text_.setPosition(toSFML(position));
}

void SFMLText::setRotation(float angle)
{
    text_.setRotation(sf::degrees(angle));
}

void SFMLText::setScale(const Vector2f& factor)
{
    text_.setScale(toSFML(factor));
}

void SFMLText::setOrigin(const Vector2f& origin)
{
    text_.setOrigin({origin.x, origin.y});
}

unsigned int SFMLText::getCharacterSize() const
{
    return text_.getCharacterSize();
}

std::string SFMLText::getString() const
{
    return text_.getString();
}

Vector2f SFMLText::getPosition() const
{
    return fromSFML(text_.getPosition());
}

FloatRect SFMLText::getGlobalBounds() const
{
    return fromSFML(text_.getGlobalBounds());
}

FloatRect SFMLText::getLocalBounds() const
{
    return fromSFML(text_.getLocalBounds());
}

const sf::Text& SFMLText::getSFMLText() const
{
    return text_;
}
