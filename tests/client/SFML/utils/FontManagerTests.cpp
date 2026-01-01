#include "TestAssets.hpp"
#include "errors/AssetLoadError.hpp"
#include "graphics/FontManager.hpp"

#include <gtest/gtest.h>

TEST(FontManager, LoadGetAndClear)
{
    FontManager manager;
    const auto& font = manager.load("test_font", assetPath("fonts/ui.ttf"));

    EXPECT_NE(&font, nullptr);

    auto fetched = manager.get("test_font");
    EXPECT_NE(fetched, nullptr);
    EXPECT_EQ(fetched.get(), &font);

    manager.clear();
    EXPECT_EQ(manager.get("test_font"), nullptr);
}

TEST(FontManager, LoadTwiceReplacesExisting)
{
    FontManager manager;
    const auto& first  = manager.load("test_font", assetPath("fonts/ui.ttf"));
    const auto& second = manager.load("test_font", assetPath("fonts/ui.ttf"));

    EXPECT_NE(&first, nullptr);
    EXPECT_NE(&second, nullptr);
    EXPECT_EQ(manager.get("test_font").get(), &second);
}

TEST(FontManager, LoadThrowsOnMissingFile)
{
    FontManager manager;
    EXPECT_THROW(manager.load("missing", assetPath("fonts/does_not_exist.ttf")), AssetLoadError);
}

TEST(FontManager, GetUnknownReturnsNullptr)
{
    FontManager manager;
    EXPECT_EQ(manager.get("unknown"), nullptr);
}

TEST(FontManager, FailedLoadDoesNotInsert)
{
    FontManager manager;
    EXPECT_THROW(manager.load("bad", assetPath("fonts/nope.ttf")), AssetLoadError);
    EXPECT_EQ(manager.get("bad"), nullptr);
}

TEST(FontManager, HasReturnsTrueForLoaded)
{
    FontManager manager;
    EXPECT_FALSE(manager.has("test_font"));
    manager.load("test_font", assetPath("fonts/ui.ttf"));
    EXPECT_TRUE(manager.has("test_font"));
}

TEST(FontManager, RemoveDeletesFont)
{
    FontManager manager;
    manager.load("test_font", assetPath("fonts/ui.ttf"));
    EXPECT_TRUE(manager.has("test_font"));

    manager.remove("test_font");
    EXPECT_FALSE(manager.has("test_font"));
    EXPECT_EQ(manager.get("test_font"), nullptr);
}

TEST(FontManager, RemoveNonexistentDoesNotCrash)
{
    FontManager manager;
    EXPECT_NO_THROW(manager.remove("nonexistent"));
}

TEST(FontManager, SizeReturnsCorrectCount)
{
    FontManager manager;
    EXPECT_EQ(manager.size(), 0u);

    manager.load("font1", assetPath("fonts/ui.ttf"));
    EXPECT_EQ(manager.size(), 1u);

    manager.load("font2", assetPath("fonts/ui.ttf"));
    EXPECT_EQ(manager.size(), 2u);

    manager.remove("font1");
    EXPECT_EQ(manager.size(), 1u);

    manager.clear();
    EXPECT_EQ(manager.size(), 0u);
}

TEST(FontManager, ReloadPreservesId)
{
    FontManager manager;
    manager.load("test_font", assetPath("fonts/ui.ttf"));
    auto first = manager.get("test_font");

    manager.load("test_font", assetPath("fonts/ui.ttf"));
    auto second = manager.get("test_font");

    EXPECT_NE(first, nullptr);
    EXPECT_NE(second, nullptr);
    EXPECT_EQ(manager.size(), 1u);
}

TEST(FontManager, MultipleFontsIndependent)
{
    FontManager manager;
    manager.load("font1", assetPath("fonts/ui.ttf"));
    manager.load("font2", assetPath("fonts/ui.ttf"));

    EXPECT_NE(manager.get("font1"), manager.get("font2"));
    EXPECT_EQ(manager.size(), 2u);

    manager.remove("font1");
    EXPECT_FALSE(manager.has("font1"));
    EXPECT_TRUE(manager.has("font2"));
}
