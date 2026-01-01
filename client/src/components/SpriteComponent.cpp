
#include "components/SpriteComponent.hpp"
#include "graphics/GraphicsFactory.hpp"

SpriteComponent::SpriteComponent(const std::shared_ptr<ITexture>& texture)
    : texture(texture)
{
    GraphicsFactory factory;
    sprite = factory.createSprite();
    if (texture) {
        sprite->setTexture(*texture);
        Vector2u size = texture->getSize();
        sprite->setTextureRect(
            IntRect{0, 0, static_cast<int>(size.x), static_cast<int>(size.y)});
    }
}

void SpriteComponent::setTexture(const std::shared_ptr<ITexture>& texture)
{
    this->texture = texture;
    if (!sprite) {
        GraphicsFactory factory;
        sprite = factory.createSprite();
    }
    if (texture) {
        sprite->setTexture(*texture);
    }
    
    if (!customFrames.empty()) {
        setFrame(currentFrame);
        return;
    }
    if (frameWidth == 0 || frameHeight == 0) {
        if (texture) {
            Vector2u size = texture->getSize();
            sprite->setTextureRect(
                IntRect{0, 0, static_cast<int>(size.x), static_cast<int>(size.y)});
        }
    } else {
        setFrame(currentFrame);
    }
}

void SpriteComponent::setPosition(float x, float y)
{
    if (sprite) {
        sprite->setPosition(Vector2f{x, y});
    }
}

void SpriteComponent::setScale(float x, float y)
{
    if (sprite) {
        sprite->setScale(Vector2f{x, y});
    }
}

void SpriteComponent::setFrameSize(std::uint32_t width, std::uint32_t height, std::uint32_t cols)
{
    frameWidth  = width;
    frameHeight = height;
    columns     = cols == 0 ? 1 : cols;
    setFrame(currentFrame);
}

void SpriteComponent::setFrame(std::uint32_t frameIndex)
{
    currentFrame = frameIndex;
    if (!sprite) {
        return;
    }
    if (!customFrames.empty()) {
        std::uint32_t idx = frameIndex % static_cast<std::uint32_t>(customFrames.size());
        sprite->setTextureRect(customFrames[idx]);
        return;
    }
    if (frameWidth == 0 || frameHeight == 0) {
        return;
    }
    std::uint32_t col = frameIndex % columns;
    std::uint32_t row = frameIndex / columns;
    std::uint32_t x   = col * frameWidth;
    std::uint32_t y   = row * frameHeight;
    sprite->setTextureRect(IntRect{static_cast<int>(x), static_cast<int>(y),
                                   static_cast<int>(frameWidth), static_cast<int>(frameHeight)});
}

std::uint32_t SpriteComponent::getFrame() const
{
    return currentFrame;
}

std::shared_ptr<ISprite> SpriteComponent::getSprite() const
{
    return sprite;
}

bool SpriteComponent::hasSprite() const
{
    return sprite != nullptr;
}
