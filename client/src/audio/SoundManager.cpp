#include "audio/SoundManager.hpp"

#include "errors/AssetLoadError.hpp"

const sf::SoundBuffer& SoundManager::load(const std::string& id, const std::string& filepath)
{
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(filepath)) {
        throw AssetLoadError("Failed to load sound: " + filepath);
    }
    buffers_[id] = std::move(buffer);
    return buffers_.at(id);
}

const sf::SoundBuffer* SoundManager::get(const std::string& id) const
{
    auto it = buffers_.find(id);
    if (it == buffers_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool SoundManager::has(const std::string& id) const
{
    return buffers_.find(id) != buffers_.end();
}

void SoundManager::remove(const std::string& id)
{
    buffers_.erase(id);
}

void SoundManager::clear()
{
    buffers_.clear();
}
