#include "components/SpriteComponent.hpp"

SpriteComponent::SpriteComponent(const sf::Texture& texture) : sprite(sf::Sprite(texture))
{
    const sf::Vector2u size = texture.getSize();
    sprite->setTextureRect(
        sf::IntRect(sf::Vector2i{0, 0}, sf::Vector2i{static_cast<int>(size.x), static_cast<int>(size.y)}));
}

void SpriteComponent::setTexture(const sf::Texture& texture)
{
    sprite.emplace(texture);
    if (frameWidth == 0 || frameHeight == 0) {
        const sf::Vector2u size = texture.getSize();
        sprite->setTextureRect(
            sf::IntRect(sf::Vector2i{0, 0}, sf::Vector2i{static_cast<int>(size.x), static_cast<int>(size.y)}));
    } else {
        setFrame(currentFrame);
    }
}

void SpriteComponent::setPosition(float x, float y)
{
    if (sprite) {
        sprite->setPosition(sf::Vector2f{x, y});
    }
}

void SpriteComponent::setScale(float x, float y)
{
    if (sprite) {
        sprite->setScale(sf::Vector2f{x, y});
    }
}

void SpriteComponent::setFrameSize(std::uint32_t width, std::uint32_t height, std::uint32_t cols)
{
    frameWidth  = width;
    frameHeight = height;
    columns     = cols;
    setFrame(currentFrame);
}

void SpriteComponent::setFrame(std::uint32_t frameIndex)
{
    currentFrame = frameIndex;
    if (!sprite || frameWidth == 0 || frameHeight == 0) {
        return;
    }
    std::uint32_t col = frameIndex % columns;
    std::uint32_t row = frameIndex / columns;
    std::uint32_t x   = col * frameWidth;
    std::uint32_t y   = row * frameHeight;
    sprite->setTextureRect(sf::IntRect(sf::Vector2i{static_cast<int>(x), static_cast<int>(y)},
                                       sf::Vector2i{static_cast<int>(frameWidth), static_cast<int>(frameHeight)}));
}

std::uint32_t SpriteComponent::getFrame() const
{
    return currentFrame;
}

const sf::Sprite* SpriteComponent::raw() const
{
    return sprite ? &(*sprite) : nullptr;
}

bool SpriteComponent::hasSprite() const
{
    return sprite.has_value();
}
