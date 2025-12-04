#pragma once

#include <optional>
#include <string>
#include <vector>

struct TextureEntry
{
    std::string id;
    std::string path;
    std::string type;
};

struct SoundEntry
{
    std::string id;
    std::string path;
    std::string type;
};

struct FontEntry
{
    std::string id;
    std::string path;
    std::string type;
};

class AssetManifest
{
  public:
    static AssetManifest fromFile(const std::string& filepath);
    static AssetManifest fromString(const std::string& json);

    const std::vector<TextureEntry>& getTextures() const;
    std::vector<TextureEntry> getTexturesByType(const std::string& type) const;
    std::optional<TextureEntry> findTextureById(const std::string& id) const;

    const std::vector<SoundEntry>& getSounds() const;
    std::vector<SoundEntry> getSoundsByType(const std::string& type) const;
    std::optional<SoundEntry> findSoundById(const std::string& id) const;

    const std::vector<FontEntry>& getFonts() const;
    std::vector<FontEntry> getFontsByType(const std::string& type) const;

  private:
    std::vector<TextureEntry> textures_;
    std::vector<SoundEntry> sounds_;
    std::vector<FontEntry> fonts_;
};
