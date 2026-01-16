#pragma once

#include "components/LivesComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "graphics/abstraction/Common.hpp"
#include "level/LevelState.hpp"
#include "network/RoomType.hpp"
#include "systems/ISystem.hpp"

#include <memory>
#include <string>

class HUDSystem : public ISystem
{
  public:
    HUDSystem(Window& window, FontManager& fonts, TextureManager& textures, std::uint32_t localPlayerId,
              RoomType gameMode);
    HUDSystem(Window& window, FontManager& fonts, TextureManager& textures, LevelState& state,
              std::uint32_t localPlayerId, RoomType gameMode);

    void update(Registry& registry, float deltaTime) override;

  private:
    void updateContent(Registry& registry, EntityId id, TextComponent& textComp) const;
    void drawLivesPips(float startX, float startY, int lives, const std::shared_ptr<ITexture>& tex,
                       const IntRect& rect) const;
    std::string formatScore(int value) const;

    Window& window_;
    FontManager& fonts_;
    TextureManager& textures_;
    LevelState* state_ = nullptr;
    std::uint32_t localPlayerId_{0};
    RoomType gameMode_{RoomType::Quickplay};
};
#include "components/ChargeMeterComponent.hpp"
