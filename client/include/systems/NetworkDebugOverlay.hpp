#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/Window.hpp"
#include "systems/ISystem.hpp"

class NetworkDebugOverlay : public ISystem
{
  public:
    NetworkDebugOverlay(Window& window, FontManager& fonts);

    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;

  private:
    Window& window_;
    FontManager& fonts_;
};
