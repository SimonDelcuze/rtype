#pragma once

#include <string>
#include <vector>

struct TextureEntry
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

  private:
    std::vector<TextureEntry> textures_;
};
