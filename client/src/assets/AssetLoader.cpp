#include "assets/AssetLoader.hpp"

AssetLoader::AssetLoader(TextureManager& textureManager) : textureManager_(textureManager) {}

void AssetLoader::loadFromManifest(const AssetManifest& manifest)
{
    loadFromManifest(manifest, nullptr);
}

void AssetLoader::loadFromManifest(const AssetManifest& manifest, ProgressCallback callback)
{
    const auto& textures = manifest.getTextures();
    std::size_t total    = textures.size();
    std::size_t loaded   = 0;

    for (const auto& entry : textures) {
        textureManager_.load(entry.id, entry.path);
        ++loaded;

        if (callback) {
            callback(loaded, total, entry.id);
        }
    }
}

void AssetLoader::loadFromManifestFile(const std::string& filepath)
{
    loadFromManifestFile(filepath, nullptr);
}

void AssetLoader::loadFromManifestFile(const std::string& filepath, ProgressCallback callback)
{
    AssetManifest manifest = AssetManifest::fromFile(filepath);
    loadFromManifest(manifest, callback);
}
