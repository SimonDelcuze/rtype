#include "graphics/TextureManager.hpp"

#include "errors/AssetLoadError.hpp"

#include <SFML/Graphics/Image.hpp>
#include <cstdlib>
#include <utility>

bool TextureManager::isHeadless() const
{
    if (!headlessChecked_) {
        const char* ci            = std::getenv("CI");
        const char* githubActions = std::getenv("GITHUB_ACTIONS");
        headlessMode_             = (ci != nullptr) || (githubActions != nullptr);
        headlessChecked_          = true;
    }
    return headlessMode_;
}

const sf::Texture& TextureManager::load(const std::string& id, const std::string& path)
{
    sf::Image image;
    if (!image.loadFromFile(path)) {
        throw AssetLoadError("Failed to load texture at path: " + path);
    }

    if (isHeadless()) {
        images_[id] = image;
        headlessIds_.insert(id);

        try {
            sf::Texture tex;
            if (!tex.loadFromImage(image)) {
                tex = sf::Texture{};
            }
            const auto it = textures_.find(id);
            if (it != textures_.end()) {
                it->second = std::move(tex);
                return it->second;
            }
            return textures_.emplace(id, std::move(tex)).first->second;
        } catch (...) {
            const auto it = textures_.find(id);
            if (it != textures_.end()) {
                return it->second;
            }
            return textures_.emplace(id, sf::Texture{}).first->second;
        }
    }

    sf::Texture texture{};
    if (!texture.loadFromImage(image)) {
        throw AssetLoadError("Failed to create texture from image: " + path);
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
    if (isHeadless()) {
        return headlessIds_.count(id) > 0;
    }
    return textures_.find(id) != textures_.end();
}

void TextureManager::remove(const std::string& id)
{
    textures_.erase(id);
    images_.erase(id);
    headlessIds_.erase(id);
}

void TextureManager::clear()
{
    textures_.clear();
    images_.clear();
    headlessIds_.clear();
}

std::size_t TextureManager::size() const
{
    if (isHeadless()) {
        return headlessIds_.size();
    }
    return textures_.size();
}

void TextureManager::createPlaceholder()
{
    if (isHeadless()) {
        return;
    }

    constexpr unsigned int kSize = 32;
    sf::Image img({kSize, kSize}, sf::Color::Magenta);
    sf::Texture tex;
    if (tex.loadFromImage(img)) {
        placeholder_ = std::move(tex);
    }
}

const sf::Texture& TextureManager::getPlaceholder()
{
    if (isHeadless()) {
        auto it = textures_.find("__dummy__");
        if (it != textures_.end()) {
            return it->second;
        }
        static sf::Texture fallback;
        return fallback;
    }

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
