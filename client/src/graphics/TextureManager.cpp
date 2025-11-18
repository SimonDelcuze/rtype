#include "graphics/TextureManager.hpp"

#include <stdexcept>
#include <utility>

const sf::Texture& TextureManager::load(const std::string& id, const std::string& path)
{
    sf::Texture texture{};
    if (!texture.loadFromFile(path)) {
        throw std::runtime_error("Failed to load texture at path: " + path);
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

void TextureManager::clear()
{
    textures_.clear();
}
