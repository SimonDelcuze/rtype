#pragma once

#include <SFML/Graphics/Font.hpp>
#include <string>
#include <unordered_map>

class FontManager
{
  public:
    const sf::Font& load(const std::string& id, const std::string& path);
    const sf::Font* get(const std::string& id) const;
    bool has(const std::string& id) const;
    void remove(const std::string& id);
    void clear();
    std::size_t size() const;

  private:
    std::unordered_map<std::string, sf::Font> fonts_;
};
