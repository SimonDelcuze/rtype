#include "components/SpriteComponent.hpp"
#include "graphics/backends/sfml/SFMLTexture.hpp"

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

TEST(SpriteComponentTest, Constructor)
{
    auto texture = std::make_shared<SFMLTexture>();
    texture->create(100, 100);

    SpriteComponent sprite(texture);

    EXPECT_EQ(sprite.texture, texture);
    EXPECT_TRUE(sprite.hasSprite());
}

TEST(SpriteComponentTest, SetTexture)
{
    SpriteComponent sprite;
    auto texture = std::make_shared<SFMLTexture>();
    texture->create(100, 100);

    sprite.setTexture(texture);

    EXPECT_EQ(sprite.texture, texture);
    EXPECT_TRUE(sprite.hasSprite());
}

TEST(SpriteComponentTest, ResetTexture)
{
    auto texture = std::make_shared<SFMLTexture>();
    texture->create(100, 100);

    SpriteComponent sprite(texture);
}

TEST(SpriteComponentTest, SetFrameSize)
{
    SpriteComponent sprite;
    sprite.setFrameSize(32, 64, 8);

    EXPECT_EQ(sprite.frameWidth, 32);
    EXPECT_EQ(sprite.frameHeight, 64);
    EXPECT_EQ(sprite.columns, 8);
}

TEST(SpriteComponentTest, SetFrame)
{
    SpriteComponent sprite;
    sprite.setFrameSize(32, 32, 4);

    sprite.setFrame(2);
    EXPECT_EQ(sprite.getFrame(), 2);

    sprite.setFrame(0);
    EXPECT_EQ(sprite.getFrame(), 0);
}

TEST(SpriteComponentTest, SetFrameUpdatesTextureRect)
{
    auto texture = std::make_shared<SFMLTexture>();
    texture->create(100, 100);

    SpriteComponent sprite;
    sprite.setTexture(texture);
    sprite.setFrameSize(32, 32, 4);

    sprite.setFrame(1);
    auto raw = sprite.getSprite();
    ASSERT_NE(raw, nullptr);
    IntRect rect = raw->getTextureRect();
    EXPECT_EQ(rect.left, 32);
    EXPECT_EQ(rect.top, 0);
    EXPECT_EQ(rect.width, 32);
    EXPECT_EQ(rect.height, 32);

    sprite.setFrame(3);
    rect = raw->getTextureRect();
    EXPECT_EQ(rect.left, 96);
    EXPECT_EQ(rect.top, 0);
}

TEST(SpriteComponentTest, SetFrameWithMultipleRows)
{
    auto texture = std::make_shared<SFMLTexture>();
    texture->create(100, 100);

    SpriteComponent sprite;
    sprite.setTexture(texture);
    sprite.setFrameSize(32, 32, 2);

    sprite.setFrame(0);
    auto raw = sprite.getSprite();
    ASSERT_NE(raw, nullptr);
    IntRect rect = raw->getTextureRect();
    EXPECT_EQ(rect.left, 0);
    EXPECT_EQ(rect.top, 0);

    sprite.setFrame(1);
    rect = raw->getTextureRect();
    EXPECT_EQ(rect.left, 32);
    EXPECT_EQ(rect.top, 0);

    sprite.setFrame(2);
    rect = raw->getTextureRect();
    EXPECT_EQ(rect.left, 0);
    EXPECT_EQ(rect.top, 32);

    sprite.setFrame(3);
    rect = raw->getTextureRect();
    EXPECT_EQ(rect.left, 32);
    EXPECT_EQ(rect.top, 32);
}

TEST(SpriteComponentTest, SetPositionAndScale)
{
    auto texture = std::make_shared<SFMLTexture>();
    texture->create(100, 100);

    SpriteComponent sprite;
    sprite.setTexture(texture);
    sprite.setPosition(100.0F, 200.0F);
    sprite.setScale(2.0F, 3.0F);

    auto raw = sprite.getSprite();
    ASSERT_NE(raw, nullptr);
    Vector2f pos   = raw->getPosition();
    Vector2f scale = raw->getScale();

    EXPECT_FLOAT_EQ(pos.x, 100.0F);
    EXPECT_FLOAT_EQ(pos.y, 200.0F);
    EXPECT_FLOAT_EQ(scale.x, 2.0F);
    EXPECT_FLOAT_EQ(scale.y, 3.0F);
}

TEST(SpriteComponentTest, SetFrameIgnoredWhenNoFrameSize)
{
    SpriteComponent sprite;
    sprite.setFrame(5);

    EXPECT_EQ(sprite.getFrame(), 5);
}

TEST(SpriteComponentTest, ConstructorWithTexture)
{
    auto texture = std::make_shared<SFMLTexture>();
    texture->create(100, 100);

    SpriteComponent sprite(texture);

    EXPECT_TRUE(sprite.hasSprite());
    auto raw = sprite.getSprite();
    ASSERT_NE(raw, nullptr);
}
