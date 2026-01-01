#include "graphics/TextureManager.hpp"

#include "errors/AssetLoadError.hpp"
#include "graphics/GraphicsFactory.hpp"

#include <utility>

const ITexture& TextureManager::load(const std::string& id, const std::string& path)
{
    GraphicsFactory factory;
    auto texture = factory.createTexture();
    if (!texture->loadFromFile(path)) {
        throw AssetLoadError("Failed to load texture at path: " + path);
    }
    const auto it = textures_.find(id);
    if (it != textures_.end()) {
        it->second = std::move(texture);
        return *it->second;
    }
    return *textures_.emplace(id, std::move(texture)).first->second;
}

std::shared_ptr<ITexture> TextureManager::get(const std::string& id) const
{
    const auto it = textures_.find(id);
    if (it == textures_.end()) {
        return nullptr;
    }
    return it->second;
}

bool TextureManager::has(const std::string& id) const
{
    return textures_.find(id) != textures_.end();
}

void TextureManager::remove(const std::string& id)
{
    textures_.erase(id);
}

void TextureManager::clear()
{
    textures_.clear();
}

std::size_t TextureManager::size() const
{
    return textures_.size();
}

void TextureManager::createPlaceholder()
{
    GraphicsFactory factory;
    placeholder_ = factory.createTexture();
}

std::shared_ptr<ITexture> TextureManager::getPlaceholder()
{
    if (!placeholder_) {
        createPlaceholder();
    }
    return placeholder_;
}

std::shared_ptr<ITexture> TextureManager::getOrPlaceholder(const std::string& id)
{
    if (auto it = textures_.find(id); it != textures_.end()) {
        return it->second;
    }
    return getPlaceholder();
}
