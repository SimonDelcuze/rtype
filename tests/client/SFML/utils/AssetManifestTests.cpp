#include "assets/AssetManifest.hpp"

#include <gtest/gtest.h>

TEST(AssetManifestTests, FromStringParsesTextures)
{
    std::string json = R"({
        "textures": [
            {"id": "player", "path": "sprites/player.png", "type": "sprite"},
            {"id": "background", "path": "backgrounds/space.png", "type": "background"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);
    const auto& textures   = manifest.getTextures();

    EXPECT_EQ(textures.size(), 2u);
    EXPECT_EQ(textures[0].id, "player");
    EXPECT_EQ(textures[0].path, "sprites/player.png");
    EXPECT_EQ(textures[0].type, "sprite");
    EXPECT_EQ(textures[1].id, "background");
    EXPECT_EQ(textures[1].path, "backgrounds/space.png");
    EXPECT_EQ(textures[1].type, "background");
}

TEST(AssetManifestTests, FromStringEmptyTexturesArray)
{
    std::string json = R"({"textures": []})";

    AssetManifest manifest = AssetManifest::fromString(json);

    EXPECT_EQ(manifest.getTextures().size(), 0u);
}

TEST(AssetManifestTests, FromStringNoTexturesField)
{
    std::string json = R"({})";

    AssetManifest manifest = AssetManifest::fromString(json);

    EXPECT_EQ(manifest.getTextures().size(), 0u);
}

TEST(AssetManifestTests, FromStringInvalidJsonThrows)
{
    std::string json = "invalid json {";

    EXPECT_THROW(AssetManifest::fromString(json), std::runtime_error);
}

TEST(AssetManifestTests, FromStringMissingIdThrows)
{
    std::string json = R"({
        "textures": [
            {"path": "sprites/player.png", "type": "sprite"}
        ]
    })";

    EXPECT_THROW(AssetManifest::fromString(json), std::runtime_error);
}

TEST(AssetManifestTests, FromStringMissingPathThrows)
{
    std::string json = R"({
        "textures": [
            {"id": "player", "type": "sprite"}
        ]
    })";

    EXPECT_THROW(AssetManifest::fromString(json), std::runtime_error);
}

TEST(AssetManifestTests, GetTexturesByTypeFiltersCorrectly)
{
    std::string json = R"({
        "textures": [
            {"id": "player", "path": "sprites/player.png", "type": "sprite"},
            {"id": "enemy", "path": "sprites/enemy.png", "type": "sprite"},
            {"id": "background", "path": "backgrounds/space.png", "type": "background"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);
    auto sprites           = manifest.getTexturesByType("sprite");
    auto backgrounds       = manifest.getTexturesByType("background");

    EXPECT_EQ(sprites.size(), 2u);
    EXPECT_EQ(backgrounds.size(), 1u);
    EXPECT_EQ(sprites[0].id, "player");
    EXPECT_EQ(sprites[1].id, "enemy");
    EXPECT_EQ(backgrounds[0].id, "background");
}

TEST(AssetManifestTests, GetTexturesByTypeNoMatch)
{
    std::string json = R"({
        "textures": [
            {"id": "player", "path": "sprites/player.png", "type": "sprite"}
        ]
    })";

    AssetManifest manifest = AssetManifest::fromString(json);
    auto sounds            = manifest.getTexturesByType("sound");

    EXPECT_EQ(sounds.size(), 0u);
}

TEST(AssetManifestTests, FromFileNonexistentThrows)
{
    EXPECT_THROW(AssetManifest::fromFile("nonexistent.json"), std::runtime_error);
}
