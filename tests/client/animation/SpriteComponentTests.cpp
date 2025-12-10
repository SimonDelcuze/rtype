#include "components/SpriteComponent.hpp"

#include <gtest/gtest.h>

TEST(SpriteComponent, DefaultValues)
{
    SpriteComponent sprite;

    EXPECT_EQ(sprite.frameWidth, 0);
    EXPECT_EQ(sprite.frameHeight, 0);
    EXPECT_EQ(sprite.columns, 1);
    EXPECT_EQ(sprite.currentFrame, 0);
    EXPECT_FALSE(sprite.hasSprite());
}

TEST(SpriteComponent, SetFrameSize)
{
    SpriteComponent sprite;
    sprite.setFrameSize(32, 64, 8);

    EXPECT_EQ(sprite.frameWidth, 32);
    EXPECT_EQ(sprite.frameHeight, 64);
    EXPECT_EQ(sprite.columns, 8);
}

TEST(SpriteComponent, SetFrame)
{
    SpriteComponent sprite;
    sprite.setFrameSize(32, 32, 4);

    sprite.setFrame(2);
    EXPECT_EQ(sprite.getFrame(), 2);

    sprite.setFrame(0);
    EXPECT_EQ(sprite.getFrame(), 0);
}

TEST(SpriteComponent, SetFrameUpdatesTextureRect)
{
    sf::Texture texture;
    ASSERT_TRUE(texture.resize({128, 32}));

    SpriteComponent sprite;
    sprite.setTexture(texture);
    sprite.setFrameSize(32, 32, 4);

    sprite.setFrame(1);
    const sf::Sprite* raw = sprite.raw();
    ASSERT_NE(raw, nullptr);
    sf::IntRect rect = raw->getTextureRect();
    EXPECT_EQ(rect.position.x, 32);
    EXPECT_EQ(rect.position.y, 0);
    EXPECT_EQ(rect.size.x, 32);
    EXPECT_EQ(rect.size.y, 32);

    sprite.setFrame(3);
    rect = raw->getTextureRect();
    EXPECT_EQ(rect.position.x, 96);
    EXPECT_EQ(rect.position.y, 0);
}

TEST(SpriteComponent, SetFrameWithMultipleRows)
{
    sf::Texture texture;
    ASSERT_TRUE(texture.resize({64, 64}));

    SpriteComponent sprite;
    sprite.setTexture(texture);
    sprite.setFrameSize(32, 32, 2);

    sprite.setFrame(0);
    const sf::Sprite* raw = sprite.raw();
    ASSERT_NE(raw, nullptr);
    sf::IntRect rect = raw->getTextureRect();
    EXPECT_EQ(rect.position.x, 0);
    EXPECT_EQ(rect.position.y, 0);

    sprite.setFrame(1);
    rect = raw->getTextureRect();
    EXPECT_EQ(rect.position.x, 32);
    EXPECT_EQ(rect.position.y, 0);

    sprite.setFrame(2);
    rect = raw->getTextureRect();
    EXPECT_EQ(rect.position.x, 0);
    EXPECT_EQ(rect.position.y, 32);

    sprite.setFrame(3);
    rect = raw->getTextureRect();
    EXPECT_EQ(rect.position.x, 32);
    EXPECT_EQ(rect.position.y, 32);
}

TEST(SpriteComponent, SetPositionAndScale)
{
    sf::Texture texture;
    ASSERT_TRUE(texture.resize({32, 32}));

    SpriteComponent sprite;
    sprite.setTexture(texture);
    sprite.setPosition(100.0F, 200.0F);
    sprite.setScale(2.0F, 3.0F);

    const sf::Sprite* raw = sprite.raw();
    ASSERT_NE(raw, nullptr);
    sf::Vector2f pos   = raw->getPosition();
    sf::Vector2f scale = raw->getScale();

    EXPECT_FLOAT_EQ(pos.x, 100.0F);
    EXPECT_FLOAT_EQ(pos.y, 200.0F);
    EXPECT_FLOAT_EQ(scale.x, 2.0F);
    EXPECT_FLOAT_EQ(scale.y, 3.0F);
}

TEST(SpriteComponent, SetFrameIgnoredWhenNoFrameSize)
{
    SpriteComponent sprite;
    sprite.setFrame(5);

    EXPECT_EQ(sprite.getFrame(), 5);
}

TEST(SpriteComponent, ConstructorWithTexture)
{
    sf::Texture texture;
    ASSERT_TRUE(texture.resize({64, 64}));

    SpriteComponent sprite(texture);

    EXPECT_TRUE(sprite.hasSprite());
    const sf::Sprite* raw = sprite.raw();
    ASSERT_NE(raw, nullptr);
}
