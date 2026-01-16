#pragma once

#include "ecs/Registry.hpp"
#include "events/EventBus.hpp"
#include "network/LobbyPackets.hpp"
#include "network/RoomType.hpp"
#include "systems/ISystem.hpp"

#include <limits>
#include <map>
#include <string>

class GameOverSystem : public ISystem
{
  public:
    GameOverSystem(EventBus& eventBus, std::uint32_t localPlayerId, RoomType gameMode,
                   const std::vector<PlayerInfo>& playerList = {});

    void initialize() override {}
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override {}

  private:
    EventBus& eventBus_;
    std::uint32_t localPlayerId_;
    RoomType gameMode_;
    std::map<std::uint32_t, std::string> playerNames_;
    bool gameOverTriggered_      = false;
    bool localPlayerDead_        = false;
    EntityId spectatingPlayerId_ = std::numeric_limits<EntityId>::max();

    EntityId findAlivePlayer(Registry& registry);
    void enableSpectateMode(Registry& registry, EntityId targetPlayerId);
};
