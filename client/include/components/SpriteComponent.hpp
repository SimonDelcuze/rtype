#pragma once

#include <optional>
#include <memory>
#include <vector>
#include "graphics/abstraction/ISprite.hpp"
#include "graphics/abstraction/ITexture.hpp"
#include "graphics/abstraction/Common.hpp"
#include "ecs/ResetValue.hpp"

struct SpriteComponent {
    std::shared_ptr<ITexture> texture;
    std::shared_ptr<ISprite> sprite;
    std::vector<IntRect> customFrames;
    std::uint32_t currentFrame{0};
    std::uint32_t frameWidth{0};
    std::uint32_t frameHeight{0};
    std::uint32_t columns{1};

    SpriteComponent() = default;
    explicit SpriteComponent(const std::shared_ptr<ITexture>& texture);

    void setTexture(const std::shared_ptr<ITexture>& texture);
    void setPosition(float x, float y);
    void setScale(float x, float y);
    void setFrame(std::uint32_t frameIndex);
    void setFrameSize(std::uint32_t width, std::uint32_t height, std::uint32_t cols = 1);
    std::uint32_t getFrame() const;
    std::shared_ptr<ISprite> getSprite() const;
    bool hasSprite() const;
};

template <> inline void resetValue<SpriteComponent>(SpriteComponent& value)
{
    value = SpriteComponent{};
}
