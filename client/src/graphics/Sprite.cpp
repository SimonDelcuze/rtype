#include "graphics/Sprite.hpp"

Sprite::Sprite(const sf::Texture& texture, const sf::IntRect& rect) : sprite_(texture, rect)
{
    if (rect.size.x == 0 || rect.size.y == 0) {
        const sf::Vector2u size = texture.getSize();
        sprite_.setTextureRect(
            sf::IntRect(sf::Vector2i{0, 0}, sf::Vector2i{static_cast<int>(size.x), static_cast<int>(size.y)}));
    }
}

void Sprite::setPosition(float x, float y)
{
    sprite_.setPosition(sf::Vector2f{x, y});
}

void Sprite::setScale(float x, float y)
{
    sprite_.setScale(sf::Vector2f{x, y});
}

void Sprite::setTextureRect(const sf::IntRect& rect)
{
    sprite_.setTextureRect(rect);
}

void Sprite::draw(Window& window) const
{
    window.draw(sprite_);
}

const sf::Sprite& Sprite::raw() const
{
    return sprite_;
}
