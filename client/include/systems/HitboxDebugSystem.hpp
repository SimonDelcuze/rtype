#pragma once

#include "ecs/Registry.hpp"
#include "graphics/Window.hpp"

#include <SFML/Graphics/Color.hpp>

class HitboxDebugSystem
{
  public:
    explicit HitboxDebugSystem(Window& window);

    void update(Registry& registry);

    void setEnabled(bool enabled);
    void setColor(const sf::Color& color);
    void setThickness(float thickness);

  private:
    bool enabled_{false};
    sf::Color color_{sf::Color::Red};
    float thickness_{1.0f};
    Window& window_;
};
