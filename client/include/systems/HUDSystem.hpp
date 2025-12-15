#pragma once

#include "components/LivesComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "systems/ISystem.hpp"

class HUDSystem : public ISystem
{
  public:
    HUDSystem(Window& window, FontManager& fonts, TextureManager& textures);

    void update(Registry& registry, float deltaTime) override;

  private:
    void updateContent(Registry& registry, EntityId id, TextComponent& textComp) const;
    void drawLivesPips(const TransformComponent& transform, const LivesComponent& lives) const;

    Window& window_;
    FontManager& fonts_;
    TextureManager& textures_;
};
#include "components/ChargeMeterComponent.hpp"
