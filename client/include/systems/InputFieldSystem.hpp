#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/Window.hpp"
#include "systems/ISystem.hpp"

#include <SFML/Window/Event.hpp>

class InputFieldSystem : public ISystem
{
  public:
    InputFieldSystem(Window& window, FontManager& fonts);

    void update(Registry& registry, float deltaTime) override;
    void handleEvent(Registry& registry, const sf::Event& event);

  private:
    void handleTextEntered(Registry& registry, char c);
    void handleBackspace(Registry& registry);
    void handleTab(Registry& registry);
    void handleClick(Registry& registry, int x, int y);

    Window& window_;
    FontManager& fonts_;
};
