#pragma once

#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "network/LobbyConnection.hpp"
#include "ui/IMenu.hpp"

#include <string>
#include <vector>

class RoomWaitingMenu : public IMenu
{
  public:
    struct Result
    {
        bool startGame    = false;
        bool leaveRoom    = false;
        std::uint32_t roomId{0};
        std::uint16_t gamePort{0};
        std::uint8_t expectedPlayerCount{0};
    };

    struct PlayerInfo
    {
        std::string name;
        bool isHost{false};
    };

    RoomWaitingMenu(FontManager& fonts, TextureManager& textures, std::uint32_t roomId, std::uint16_t gamePort,
                    bool isHost, LobbyConnection* lobbyConnection);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry& registry) const
    {
        (void) registry;
        return result_;
    }

  private:
    void onStartGameClicked();
    void onLeaveRoomClicked();
    void updatePlayerList(Registry& registry);

    FontManager& fonts_;
    TextureManager& textures_;
    LobbyConnection* lobbyConnection_;
    Result result_;
    bool done_{false};

    std::uint32_t roomId_;
    std::uint16_t gamePort_;
    bool isHost_;
    std::vector<PlayerInfo> players_;

    EntityId backgroundEntity_{0};
    EntityId logoEntity_{0};
    EntityId titleEntity_{0};
    EntityId playerCountEntity_{0};
    EntityId startButtonEntity_{0};
    EntityId leaveButtonEntity_{0};
    std::vector<EntityId> playerTextEntities_;

    float updateTimer_{0.0F};
    constexpr static float kUpdateInterval = 1.0F;
};
