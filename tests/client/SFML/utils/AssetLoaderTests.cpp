#include "TestAssets.hpp"
#include "assets/AssetLoader.hpp"
#include "assets/AssetManifest.hpp"
#include "graphics/TextureManager.hpp"

#include <gtest/gtest.h>

TEST(AssetLoaderTests, LoadFromManifestLoadsTextures)
{
    TextureManager manager;
    AssetLoader loader(manager);

    std::string json = R"({
        "textures": [
            {"id": "space", "path": ")" +
                       assetPath("backgrounds/space.png") + R"(", "type": "background"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);
    EXPECT_NO_THROW(loader.loadFromManifest(manifest));

    EXPECT_TRUE(manager.has("space"));
    EXPECT_NE(manager.get("space"), nullptr);
}

TEST(AssetLoaderTests, LoadFromManifestMultipleTextures)
{
    TextureManager manager;
    AssetLoader loader(manager);

    std::string json = R"({
        "textures": [
            {"id": "space1", "path": ")" +
                       assetPath("backgrounds/space.png") + R"(", "type": "background"},
            {"id": "space2", "path": ")" +
                       assetPath("backgrounds/space.png") + R"(", "type": "background"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);
    loader.loadFromManifest(manifest);

    EXPECT_TRUE(manager.has("space1"));
    EXPECT_TRUE(manager.has("space2"));
    EXPECT_EQ(manager.size(), 2u);
}

TEST(AssetLoaderTests, LoadFromManifestWithCallback)
{
    TextureManager manager;
    AssetLoader loader(manager);

    std::string json = R"({
        "textures": [
            {"id": "space1", "path": ")" +
                       assetPath("backgrounds/space.png") + R"(", "type": "background"},
            {"id": "space2", "path": ")" +
                       assetPath("backgrounds/space.png") + R"(", "type": "background"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);

    std::size_t callbackCount = 0;
    std::size_t lastLoaded    = 0;
    std::size_t lastTotal     = 0;

    loader.loadFromManifest(manifest, [&](std::size_t loaded, std::size_t total, const std::string& id) {
        callbackCount++;
        lastLoaded = loaded;
        lastTotal  = total;
        (void) id;
    });

    EXPECT_EQ(callbackCount, 2u);
    EXPECT_EQ(lastLoaded, 2u);
    EXPECT_EQ(lastTotal, 2u);
}

TEST(AssetLoaderTests, LoadFromManifestInvalidPathThrows)
{
    TextureManager manager;
    AssetLoader loader(manager);

    std::string json = R"({
        "textures": [
            {"id": "invalid", "path": "nonexistent.png", "type": "sprite"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);
    EXPECT_THROW(loader.loadFromManifest(manifest), std::runtime_error);
}

TEST(AssetLoaderTests, LoadFromManifestFileNonexistent)
{
    TextureManager manager;
    AssetLoader loader(manager);

    EXPECT_THROW(loader.loadFromManifestFile("nonexistent.json"), std::runtime_error);
}

TEST(AssetLoaderTests, LoadFromManifestEmptyDoesNotCrash)
{
    TextureManager manager;
    AssetLoader loader(manager);

    std::string json = R"({"textures": []})";

    AssetManifest manifest = AssetManifest::fromString(json);
    EXPECT_NO_THROW(loader.loadFromManifest(manifest));
    EXPECT_EQ(manager.size(), 0u);
}
