#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/Window.hpp"
#include "systems/ISystem.hpp"

#include <SFML/Window/Event.hpp>

class ButtonSystem : public ISystem
{
  public:
    ButtonSystem(Window& window, FontManager& fonts);

    void update(Registry& registry, float deltaTime) override;
    void handleEvent(Registry& registry, const sf::Event& event);

  private:
    void handleClick(Registry& registry, int x, int y);
    void handleMouseMove(Registry& registry, int x, int y);

    Window& window_;
    FontManager& fonts_;
};
