#pragma once

#include "assets/AssetManifest.hpp"
#include "audio/SoundManager.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"

#include <functional>
#include <string>

class AssetLoader
{
  public:
    using ProgressCallback = std::function<void(std::size_t loaded, std::size_t total, const std::string& currentId)>;

    AssetLoader(TextureManager& textureManager, SoundManager& soundManager, FontManager& fontManager);

    void loadFromManifest(const AssetManifest& manifest);
    void loadFromManifest(const AssetManifest& manifest, ProgressCallback callback);
    void loadFromManifestFile(const std::string& filepath);
    void loadFromManifestFile(const std::string& filepath, ProgressCallback callback);

  private:
    TextureManager& textureManager_;
    SoundManager& soundManager_;
    FontManager& fontManager_;
};
