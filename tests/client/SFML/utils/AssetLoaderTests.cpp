#include "TestAssets.hpp"
#include "assets/AssetLoader.hpp"
#include "assets/AssetManifest.hpp"
#include "audio/SoundManager.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"

#include <gtest/gtest.h>

TEST(AssetLoaderTests, LoadFromManifestLoadsTextures)
{
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

    std::string json = R"({
        "textures": [
            {"id": "space", "path": ")" +
                       assetPath("backgrounds/space.png") + R"(", "type": "background"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);
    EXPECT_NO_THROW(loader.loadFromManifest(manifest));

    EXPECT_TRUE(textureManager.has("space"));
    EXPECT_NE(textureManager.get("space"), nullptr);
}

TEST(AssetLoaderTests, LoadFromManifestMultipleTextures)
{
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

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

    EXPECT_TRUE(textureManager.has("space1"));
    EXPECT_TRUE(textureManager.has("space2"));
    EXPECT_EQ(textureManager.size(), 2u);
}

TEST(AssetLoaderTests, LoadFromManifestWithCallback)
{
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

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
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

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
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

    EXPECT_THROW(loader.loadFromManifestFile("nonexistent.json"), std::runtime_error);
}

TEST(AssetLoaderTests, LoadFromManifestEmptyDoesNotCrash)
{
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

    std::string json = R"({"textures": []})";

    AssetManifest manifest = AssetManifest::fromString(json);
    EXPECT_NO_THROW(loader.loadFromManifest(manifest));
    EXPECT_EQ(textureManager.size(), 0u);
}

TEST(AssetLoaderTests, LoadFromManifestLoadsSounds)
{
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

    std::string json = R"({
        "sounds": [
            {"id": "test_sound", "path": ")" +
                       assetPath("sounds/beep.wav") + R"(", "type": "sfx"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);
    EXPECT_NO_THROW(loader.loadFromManifest(manifest));

    EXPECT_TRUE(soundManager.has("test_sound"));
    EXPECT_NE(soundManager.get("test_sound"), nullptr);
}

TEST(AssetLoaderTests, LoadFromManifestTexturesAndSounds)
{
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

    std::string json = R"({
        "textures": [
            {"id": "tex1", "path": ")" +
                       assetPath("backgrounds/space.png") + R"(", "type": "background"}
        ],
        "sounds": [
            {"id": "snd1", "path": ")" +
                       assetPath("sounds/beep.wav") + R"(", "type": "sfx"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);
    loader.loadFromManifest(manifest);

    EXPECT_TRUE(textureManager.has("tex1"));
    EXPECT_TRUE(soundManager.has("snd1"));
}

TEST(AssetLoaderTests, LoadFromManifestCallbackCountsTexturesAndSounds)
{
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

    std::string json = R"({
        "textures": [
            {"id": "tex1", "path": ")" +
                       assetPath("backgrounds/space.png") + R"(", "type": "background"}
        ],
        "sounds": [
            {"id": "snd1", "path": ")" +
                       assetPath("sounds/beep.wav") + R"(", "type": "sfx"},
            {"id": "snd2", "path": ")" +
                       assetPath("sounds/beep.wav") + R"(", "type": "sfx"}
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

    EXPECT_EQ(callbackCount, 3u);
    EXPECT_EQ(lastLoaded, 3u);
    EXPECT_EQ(lastTotal, 3u);
}

TEST(AssetLoaderTests, LoadFromManifestLoadsFonts)
{
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

    std::string json = R"({
        "fonts": [
            {"id": "test_font", "path": ")" +
                       assetPath("fonts/test.ttf") + R"(", "type": "ui"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);
    EXPECT_NO_THROW(loader.loadFromManifest(manifest));

    EXPECT_TRUE(fontManager.has("test_font"));
    EXPECT_NE(fontManager.get("test_font"), nullptr);
}

TEST(AssetLoaderTests, LoadFromManifestAllAssetTypes)
{
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

    std::string json = R"({
        "textures": [
            {"id": "tex1", "path": ")" +
                       assetPath("backgrounds/space.png") + R"(", "type": "background"}
        ],
        "sounds": [
            {"id": "snd1", "path": ")" +
                       assetPath("sounds/beep.wav") + R"(", "type": "sfx"}
        ],
        "fonts": [
            {"id": "fnt1", "path": ")" +
                       assetPath("fonts/test.ttf") + R"(", "type": "ui"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);
    loader.loadFromManifest(manifest);

    EXPECT_TRUE(textureManager.has("tex1"));
    EXPECT_TRUE(soundManager.has("snd1"));
    EXPECT_TRUE(fontManager.has("fnt1"));
}

TEST(AssetLoaderTests, LoadFromManifestCallbackCountsAllAssets)
{
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

    std::string json = R"({
        "textures": [
            {"id": "tex1", "path": ")" +
                       assetPath("backgrounds/space.png") + R"(", "type": "background"}
        ],
        "sounds": [
            {"id": "snd1", "path": ")" +
                       assetPath("sounds/beep.wav") + R"(", "type": "sfx"}
        ],
        "fonts": [
            {"id": "fnt1", "path": ")" +
                       assetPath("fonts/test.ttf") + R"(", "type": "ui"},
            {"id": "fnt2", "path": ")" +
                       assetPath("fonts/test.ttf") + R"(", "type": "game"}
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

    EXPECT_EQ(callbackCount, 4u);
    EXPECT_EQ(lastLoaded, 4u);
    EXPECT_EQ(lastTotal, 4u);
}
