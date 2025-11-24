#pragma once

#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/Window.hpp"
#include "systems/ISystem.hpp"

#include <vector>

class RenderSystem : public ISystem
{
  public:
    explicit RenderSystem(Window& window);

    void update(Registry& registry, float deltaTime) override;

  private:
    Window& window_;
};
