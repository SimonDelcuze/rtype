#include "TestAssets.hpp"
#include "graphics/TextureManager.hpp"

#include <gtest/gtest.h>

TEST(TextureManager, LoadGetAndClear)
{
    TextureManager manager;
    const auto& texture = manager.load("background", assetPath("backgrounds/space.png"));

    EXPECT_GT(texture.getSize().x, 0u);
    EXPECT_GT(texture.getSize().y, 0u);

    const sf::Texture* fetched = manager.get("background");
    EXPECT_NE(fetched, nullptr);
    EXPECT_EQ(fetched, &texture);

    manager.clear();
    EXPECT_EQ(manager.get("background"), nullptr);
}

TEST(TextureManager, LoadTwiceReplacesExisting)
{
    TextureManager manager;
    const auto& first  = manager.load("background", assetPath("backgrounds/space.png"));
    const auto& second = manager.load("background", assetPath("backgrounds/space.png"));

    EXPECT_NE(&first, nullptr);
    EXPECT_NE(&second, nullptr);
    EXPECT_EQ(manager.get("background"), &second);
}

TEST(TextureManager, LoadThrowsOnMissingFile)
{
    TextureManager manager;
    EXPECT_THROW(manager.load("missing", assetPath("backgrounds/does_not_exist.png")), std::runtime_error);
}

TEST(TextureManager, GetUnknownReturnsNullptr)
{
    TextureManager manager;
    EXPECT_EQ(manager.get("unknown"), nullptr);
}

TEST(TextureManager, FailedLoadDoesNotInsert)
{
    TextureManager manager;
    EXPECT_THROW(manager.load("bad", assetPath("backgrounds/nope.png")), std::runtime_error);
    EXPECT_EQ(manager.get("bad"), nullptr);
}
