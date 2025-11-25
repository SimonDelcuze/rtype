#pragma once

#include "assets/AssetManifest.hpp"
#include "graphics/TextureManager.hpp"

#include <functional>
#include <string>

class AssetLoader
{
  public:
    using ProgressCallback = std::function<void(std::size_t loaded, std::size_t total, const std::string& currentId)>;

    explicit AssetLoader(TextureManager& textureManager);

    void loadFromManifest(const AssetManifest& manifest);
    void loadFromManifest(const AssetManifest& manifest, ProgressCallback callback);
    void loadFromManifestFile(const std::string& filepath);
    void loadFromManifestFile(const std::string& filepath, ProgressCallback callback);

  private:
    TextureManager& textureManager_;
};
