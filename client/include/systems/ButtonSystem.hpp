#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/Window.hpp"
#include "graphics/abstraction/Event.hpp"
#include "systems/ISystem.hpp"

class ButtonSystem : public ISystem
{
  public:
    ButtonSystem(Window& window, FontManager& fonts);

    void update(Registry& registry, float deltaTime) override;
    void handleEvent(Registry& registry, const Event& event);

  private:
    void handleClick(Registry& registry, int x, int y);
    void handleMouseMove(Registry& registry, int x, int y);

    Window& window_;
    FontManager& fonts_;
};
