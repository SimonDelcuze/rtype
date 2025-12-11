#pragma once

#include "ecs/Registry.hpp"
#include "systems/ISystem.hpp"
#include "graphics/Window.hpp"

#include <SFML/Graphics/Color.hpp>

class HitboxDebugSystem : public ISystem
{
  public:
    explicit HitboxDebugSystem(Window& window);

    void update(Registry& registry, float deltaTime = 0.0F) override;

    void setEnabled(bool enabled);
    void setColor(const sf::Color& color);
    void setThickness(float thickness);

  private:
    Window& window_;
    bool enabled_    = false;
    sf::Color color_ = sf::Color::Green;
    float thickness_ = 1.0F;
};
