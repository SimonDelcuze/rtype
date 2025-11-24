#pragma once

#include <SFML/Audio/SoundBuffer.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>

class SoundManager
{
  public:
    const sf::SoundBuffer& load(const std::string& id, const std::string& filepath);
    const sf::SoundBuffer* get(const std::string& id) const;
    bool has(const std::string& id) const;
    void remove(const std::string& id);
    void clear();

  private:
    std::unordered_map<std::string, sf::SoundBuffer> buffers_;
};
