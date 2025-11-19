#pragma once

#include "graphics/Window.hpp"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

class Sprite
{
  public:
    Sprite(const sf::Texture& texture, const sf::IntRect& rect = {});

    void setPosition(float x, float y);
    void setScale(float x, float y);
    void setTextureRect(const sf::IntRect& rect);

    void draw(Window& window) const;
    const sf::Sprite& raw() const;

  private:
    sf::Sprite sprite_;
};
