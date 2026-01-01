#include "graphics/FontManager.hpp"

#include "errors/AssetLoadError.hpp"
#include "graphics/GraphicsFactory.hpp"

#include <utility>

const IFont& FontManager::load(const std::string& id, const std::string& path)
{
    GraphicsFactory factory;
    auto font = factory.createFont();
    if (!font->loadFromFile(path)) {
        throw AssetLoadError("Failed to load font at path: " + path);
    }
    const auto it = fonts_.find(id);
    if (it != fonts_.end()) {
        it->second = std::move(font);
        return *it->second;
    }
    return *fonts_.emplace(id, std::move(font)).first->second;
}

std::shared_ptr<IFont> FontManager::get(const std::string& id) const
{
    const auto it = fonts_.find(id);
    if (it == fonts_.end()) {
        return nullptr;
    }
    return it->second;
}

bool FontManager::has(const std::string& id) const
{
    return fonts_.find(id) != fonts_.end();
}

void FontManager::remove(const std::string& id)
{
    fonts_.erase(id);
}

void FontManager::clear()
{
    fonts_.clear();
}

std::size_t FontManager::size() const
{
    return fonts_.size();
}
