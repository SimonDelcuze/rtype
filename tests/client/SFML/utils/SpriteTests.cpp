#include <gtest/gtest.h>

#include "graphics/Sprite.hpp"
#include "graphics/TextureManager.hpp"
#include <filesystem>
#include <string>

namespace {
    std::string assetPath(const std::string& relative)
    {
        return (std::filesystem::path(RTYPE_ASSETS_DIR) / relative).string();
    }
}

TEST(Sprite, DefaultsToFullTextureRect)
{
    TextureManager manager;
    const auto& texture = manager.load("background", assetPath("backgrounds/space.png"));

    Sprite sprite(texture);
    const sf::IntRect rect = sprite.raw().getTextureRect();

    EXPECT_EQ(rect.position.x, 0);
    EXPECT_EQ(rect.position.y, 0);
    EXPECT_EQ(rect.size.x, static_cast<int>(texture.getSize().x));
    EXPECT_EQ(rect.size.y, static_cast<int>(texture.getSize().y));
}

TEST(Sprite, CustomTextureRect)
{
    TextureManager manager;
    const auto& texture = manager.load("background", assetPath("backgrounds/space.png"));

    const sf::IntRect rect(sf::Vector2i{10, 5}, sf::Vector2i{50, 25});
    Sprite sprite(texture, rect);

    const sf::IntRect fetched = sprite.raw().getTextureRect();
    EXPECT_EQ(fetched.position.x, rect.position.x);
    EXPECT_EQ(fetched.position.y, rect.position.y);
    EXPECT_EQ(fetched.size.x, rect.size.x);
    EXPECT_EQ(fetched.size.y, rect.size.y);
}

TEST(Sprite, PositionAndScale)
{
    TextureManager manager;
    const auto& texture = manager.load("background", assetPath("backgrounds/space.png"));

    Sprite sprite(texture);
    sprite.setPosition(42.0F, 24.0F);
    sprite.setScale(2.0F, 3.0F);

    const sf::Vector2f pos = sprite.raw().getPosition();
    const sf::Vector2f scale = sprite.raw().getScale();

    EXPECT_FLOAT_EQ(pos.x, 42.0F);
    EXPECT_FLOAT_EQ(pos.y, 24.0F);
    EXPECT_FLOAT_EQ(scale.x, 2.0F);
    EXPECT_FLOAT_EQ(scale.y, 3.0F);
}

TEST(Sprite, ZeroSizeRectFallsBackToTexture)
{
    TextureManager manager;
    const auto& texture = manager.load("background", assetPath("backgrounds/space.png"));
    const sf::IntRect zeroRect(sf::Vector2i{0, 0}, sf::Vector2i{0, 0});

    Sprite sprite(texture, zeroRect);
    const sf::IntRect rect = sprite.raw().getTextureRect();

    EXPECT_EQ(rect.size.x, static_cast<int>(texture.getSize().x));
    EXPECT_EQ(rect.size.y, static_cast<int>(texture.getSize().y));
}
