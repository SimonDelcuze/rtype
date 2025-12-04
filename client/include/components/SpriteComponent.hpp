#pragma once

#include "ecs/ResetValue.hpp"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <cstdint>
#include <optional>
#include <vector>

struct SpriteComponent
{
    std::optional<sf::Sprite> sprite;
    std::uint32_t frameWidth   = 0;
    std::uint32_t frameHeight  = 0;
    std::uint32_t columns      = 1;
    std::uint32_t currentFrame = 0;
    std::vector<sf::IntRect> customFrames;

    SpriteComponent() = default;
    explicit SpriteComponent(const sf::Texture& texture);

    void setTexture(const sf::Texture& texture);
    void setPosition(float x, float y);
    void setScale(float x, float y);
    void setFrame(std::uint32_t frameIndex);
    void setFrameSize(std::uint32_t width, std::uint32_t height, std::uint32_t cols = 1);
    std::uint32_t getFrame() const;
    const sf::Sprite* raw() const;
    bool hasSprite() const;
};

template <> inline void resetValue<SpriteComponent>(SpriteComponent& value)
{
    value = SpriteComponent{};
}
