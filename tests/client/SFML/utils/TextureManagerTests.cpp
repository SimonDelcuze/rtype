#include "TestAssets.hpp"
#include "errors/AssetLoadError.hpp"
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
    EXPECT_THROW(manager.load("missing", assetPath("backgrounds/does_not_exist.png")), AssetLoadError);
}

TEST(TextureManager, GetUnknownReturnsNullptr)
{
    TextureManager manager;
    EXPECT_EQ(manager.get("unknown"), nullptr);
}

TEST(TextureManager, FailedLoadDoesNotInsert)
{
    TextureManager manager;
    EXPECT_THROW(manager.load("bad", assetPath("backgrounds/nope.png")), AssetLoadError);
    EXPECT_EQ(manager.get("bad"), nullptr);
}

TEST(TextureManager, HasReturnsTrueForLoaded)
{
    TextureManager manager;
    EXPECT_FALSE(manager.has("background"));
    manager.load("background", assetPath("backgrounds/space.png"));
    EXPECT_TRUE(manager.has("background"));
}

TEST(TextureManager, RemoveDeletesTexture)
{
    TextureManager manager;
    manager.load("background", assetPath("backgrounds/space.png"));
    EXPECT_TRUE(manager.has("background"));

    manager.remove("background");
    EXPECT_FALSE(manager.has("background"));
    EXPECT_EQ(manager.get("background"), nullptr);
}

TEST(TextureManager, RemoveNonexistentDoesNotCrash)
{
    TextureManager manager;
    EXPECT_NO_THROW(manager.remove("nonexistent"));
}

TEST(TextureManager, SizeReturnsCorrectCount)
{
    TextureManager manager;
    EXPECT_EQ(manager.size(), 0u);

    manager.load("tex1", assetPath("backgrounds/space.png"));
    EXPECT_EQ(manager.size(), 1u);

    manager.load("tex2", assetPath("backgrounds/space.png"));
    EXPECT_EQ(manager.size(), 2u);

    manager.remove("tex1");
    EXPECT_EQ(manager.size(), 1u);

    manager.clear();
    EXPECT_EQ(manager.size(), 0u);
}

TEST(TextureManager, ReloadPreservesId)
{
    TextureManager manager;
    manager.load("background", assetPath("backgrounds/space.png"));
    sf::Vector2u firstSize = manager.get("background")->getSize();

    manager.load("background", assetPath("backgrounds/space.png"));
    sf::Vector2u secondSize = manager.get("background")->getSize();

    EXPECT_EQ(firstSize, secondSize);
    EXPECT_EQ(manager.size(), 1u);
}

TEST(TextureManager, MultipleTexturesIndependent)
{
    TextureManager manager;
    manager.load("tex1", assetPath("backgrounds/space.png"));
    manager.load("tex2", assetPath("backgrounds/space.png"));

    EXPECT_NE(manager.get("tex1"), manager.get("tex2"));
    EXPECT_EQ(manager.size(), 2u);

    manager.remove("tex1");
    EXPECT_FALSE(manager.has("tex1"));
    EXPECT_TRUE(manager.has("tex2"));
}
