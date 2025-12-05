#pragma once

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

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
    bool isHeadless() const;

    std::unordered_map<std::string, sf::Texture> textures_;
    std::unordered_map<std::string, sf::Image> images_;
    std::unordered_set<std::string> headlessIds_;
    std::optional<sf::Texture> placeholder_;
    mutable bool headlessMode_{false};
    mutable bool headlessChecked_{false};
};
