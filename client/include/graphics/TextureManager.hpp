#pragma once

#include <SFML/Graphics/Texture.hpp>
#include <string>
#include <unordered_map>

class TextureManager
{
  public:
    const sf::Texture& load(const std::string& id, const std::string& path);
    const sf::Texture* get(const std::string& id) const;
    void clear();

  private:
    std::unordered_map<std::string, sf::Texture> textures_;
};
