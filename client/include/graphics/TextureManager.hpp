#pragma once

#include <SFML/Graphics/Texture.hpp>
#include <optional>
#include <string>
#include <unordered_map>

class TextureManager
{
  public:
    const sf::Texture& load(const std::string& id, const std::string& path);
    const sf::Texture* get(const std::string& id) const;
    bool has(const std::string& id) const;
    void remove(const std::string& id);
    void clear();
    std::size_t size() const;

    const sf::Texture& getPlaceholder();
    const sf::Texture& getOrPlaceholder(const std::string& id);

  private:
    void createPlaceholder();

    std::unordered_map<std::string, sf::Texture> textures_;
    std::optional<sf::Texture> placeholder_;
};
