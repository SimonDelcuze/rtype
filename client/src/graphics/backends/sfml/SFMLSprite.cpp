#include "graphics/backends/sfml/SFMLSprite.hpp"
#include "graphics/backends/sfml/SFMLTexture.hpp"
#include "graphics/backends/sfml/SFMLCommon.hpp"

static const sf::Texture& getDummyTexture() {
    static const sf::Texture texture;
    return texture;
}

SFMLSprite::SFMLSprite() : sprite_(getDummyTexture()) {
}

void SFMLSprite::setTexture(const ITexture& texture, bool resetRect) {
    const auto* sfmlTexture = dynamic_cast<const SFMLTexture*>(&texture);
    if (sfmlTexture) {
        sprite_.setTexture(sfmlTexture->getSFMLTexture(), resetRect);
    }
}

void SFMLSprite::setTextureRect(const IntRect& rect) {
    sprite_.setTextureRect(toSFML(rect));
}

void SFMLSprite::setPosition(const Vector2f& position) {
    sprite_.setPosition(toSFML(position));
}

void SFMLSprite::setRotation(float angle) {
    sprite_.setRotation(sf::degrees(angle));
}

void SFMLSprite::setScale(const Vector2f& factor) {
    sprite_.setScale(toSFML(factor));
}

void SFMLSprite::setOrigin(const Vector2f& origin) {
    sprite_.setOrigin(toSFML(origin));
}

void SFMLSprite::setColor(const Color& color) {
    sprite_.setColor(toSFML(color));
}

Color SFMLSprite::getColor() const
{
    return fromSFML(sprite_.getColor());
}

IntRect SFMLSprite::getTextureRect() const
{
    sf::IntRect r = sprite_.getTextureRect();
    return {r.position.x, r.position.y, r.size.x, r.size.y};
}

Vector2f SFMLSprite::getPosition() const {
    return fromSFML(sprite_.getPosition());
}

float SFMLSprite::getRotation() const {
    return sprite_.getRotation().asDegrees();
}

Vector2f SFMLSprite::getScale() const {
    return fromSFML(sprite_.getScale());
}

Vector2f SFMLSprite::getOrigin() const {
    return fromSFML(sprite_.getOrigin());
}

FloatRect SFMLSprite::getGlobalBounds() const {
    return fromSFML(sprite_.getGlobalBounds());
}

void SFMLSprite::move(const Vector2f& offset) {
    sprite_.move(toSFML(offset));
}

void SFMLSprite::rotate(float angle) {
    sprite_.rotate(sf::degrees(angle));
}

void SFMLSprite::scale(const Vector2f& factor) {
    sprite_.scale(toSFML(factor));
}

const sf::Sprite& SFMLSprite::getSFMLSprite() const {
    return sprite_;
}
