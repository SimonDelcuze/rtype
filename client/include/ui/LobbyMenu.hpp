#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "network/LobbyConnection.hpp"
#include "network/LobbyPackets.hpp"
#include "ui/IMenu.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

class LobbyMenu : public IMenu
{
  public:
    struct Result
    {
        bool success       = false;
        bool exitRequested = false;
        bool backRequested = false;
        std::uint32_t roomId{0};
        std::uint16_t gamePort{0};
    };

    LobbyMenu(FontManager& fonts, TextureManager& textures, const IpEndpoint& lobbyEndpoint,
              ThreadSafeQueue<std::string>& broadcastQueue, const std::atomic<bool>& runningFlag);

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
    void refreshRoomList();
    void onCreateRoomClicked();
    void onJoinRoomClicked(std::size_t roomIndex);
    void onRefreshClicked();
    void onBackClicked();
    void updateRoomListDisplay(Registry& registry);
    void createRoomButton(Registry& registry, const RoomInfo& room, std::size_t index);

    enum class State
    {
        Loading,
        ShowingRooms,
        Creating,
        Joining,
        Done
    };

    FontManager& fonts_;
    TextureManager& textures_;
    IpEndpoint lobbyEndpoint_;
    ThreadSafeQueue<std::string>& broadcastQueue_;
    const std::atomic<bool>& runningFlag_;
    std::unique_ptr<LobbyConnection> lobbyConnection_;
    Result result_;
    State state_{State::Loading};

    std::vector<RoomInfo> rooms_;
    std::vector<EntityId> roomButtonEntities_;

    EntityId backgroundEntity_{0};
    EntityId logoEntity_{0};
    EntityId titleEntity_{0};
    EntityId statusEntity_{0};
    EntityId createButtonEntity_{0};
    EntityId refreshButtonEntity_{0};
    EntityId backButtonEntity_{0};

    float refreshTimer_{0.0F};
    constexpr static float kRefreshInterval = 2.0F;
};
