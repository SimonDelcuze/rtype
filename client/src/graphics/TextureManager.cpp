#include "graphics/TextureManager.hpp"

#include "errors/AssetLoadError.hpp"

#include <SFML/Graphics/Image.hpp>
#include <utility>

const sf::Texture& TextureManager::load(const std::string& id, const std::string& path)
{
    sf::Texture texture{};
    if (!texture.loadFromFile(path)) {
        throw AssetLoadError("Failed to load texture at path: " + path);
    }
    const auto it = textures_.find(id);
    if (it != textures_.end()) {
        it->second = std::move(texture);
        return it->second;
    }
    return textures_.emplace(id, std::move(texture)).first->second;
}

const sf::Texture* TextureManager::get(const std::string& id) const
{
    const auto it = textures_.find(id);
    if (it == textures_.end()) {
        return nullptr;
    }
    return &it->second;
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
    constexpr unsigned int kSize = 32;
    sf::Image img({kSize, kSize}, sf::Color::Magenta);
    sf::Texture tex;
    if (tex.loadFromImage(img)) {
        placeholder_ = std::move(tex);
    }
}

const sf::Texture& TextureManager::getPlaceholder()
{
    if (!placeholder_) {
        createPlaceholder();
    }
    return *placeholder_;
}

const sf::Texture& TextureManager::getOrPlaceholder(const std::string& id)
{
    auto* tex = get(id);
    if (tex != nullptr) {
        return *tex;
    }
    return getPlaceholder();
}
