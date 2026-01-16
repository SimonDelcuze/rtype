#pragma once

#include "events/EventBus.hpp"
#include "network/RoomType.hpp"
#include "systems/ISystem.hpp"

#include <cstdint>
#include <limits>

using EntityId = std::uint32_t;
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
    bool localPlayerDead_ = false;
    EntityId spectatingPlayerId_ = std::numeric_limits<EntityId>::max();

    EntityId findAlivePlayer(Registry& registry);
    void enableSpectateMode(Registry& registry, EntityId targetPlayerId);
};
