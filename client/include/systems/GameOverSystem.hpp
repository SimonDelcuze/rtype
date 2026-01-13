#pragma once

#include "events/EventBus.hpp"
#include "network/RoomType.hpp"
#include "systems/ISystem.hpp"

class Registry;

class GameOverSystem : public ISystem
{
  public:
    GameOverSystem(EventBus& eventBus, std::uint32_t localPlayerId, RoomType gameMode);

    void initialize() override {}
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override {}

  private:
    EventBus& eventBus_;
    std::uint32_t localPlayerId_;
    RoomType gameMode_;
    bool gameOverTriggered_ = false;
};
