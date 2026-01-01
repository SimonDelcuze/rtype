#include "audio/SoundManager.hpp"

#include "errors/AssetLoadError.hpp"
#include "graphics/GraphicsFactory.hpp"

#include <SFML/Audio/Listener.hpp>
#include <utility>

void SoundManager::setGlobalVolume(float volume)
{
    sf::Listener::setGlobalVolume(volume);
}

const ISoundBuffer& SoundManager::load(const std::string& id, const std::string& filepath)
{
    GraphicsFactory factory;
    auto buffer = factory.createSoundBuffer();
    if (!buffer->loadFromFile(filepath)) {
        throw AssetLoadError("Failed to load sound: " + filepath);
    }
    const auto it = buffers_.find(id);
    if (it != buffers_.end()) {
        it->second = std::move(buffer);
        return *it->second;
    }
    return *buffers_.emplace(id, std::move(buffer)).first->second;
}

std::shared_ptr<ISoundBuffer> SoundManager::get(const std::string& id) const
{
    auto it = buffers_.find(id);
    if (it == buffers_.end()) {
        return nullptr;
    }
    return it->second;
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
