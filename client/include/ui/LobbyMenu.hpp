#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "network/LobbyConnection.hpp"
#include "network/LobbyPackets.hpp"
#include "ui/CreateRoomMenu.hpp"
#include "ui/IMenu.hpp"
#include "ui/NotificationData.hpp"
#include "ui/PasswordInputMenu.hpp"
#include "ui/RoomWaitingMenu.hpp"

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
        bool isHost        = false;
        bool serverLost    = false;
        std::uint32_t roomId{0};
        std::uint16_t gamePort{0};
        std::uint8_t expectedPlayerCount{0};
    };

    LobbyMenu(FontManager& fonts, TextureManager& textures, const IpEndpoint& lobbyEndpoint,
              ThreadSafeQueue<NotificationData>& broadcastQueue, const std::atomic<bool>& runningFlag,
              LobbyConnection* sharedConnection = nullptr);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void update(Registry& registry, float dt) override;
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
    void onToggleFilterFull();
    void onToggleFilterProtected();
    void updateRoomListDisplay(Registry& registry);
    void createRoomButton(Registry& registry, const RoomInfo& room, std::size_t index);
    bool shouldShowRoom(const RoomInfo& room) const;
    void loadAndDisplayStats(Registry& registry);

    LobbyConnection* getConnection()
    {
        return sharedConnection_ ? sharedConnection_ : lobbyConnection_.get();
    }

    enum class State
    {
        Loading,
        ShowingRooms,
        ShowingCreateMenu,
        ShowingPasswordInput,
        Creating,
        Joining,
        InRoom,
        Done
    };

    FontManager& fonts_;
    TextureManager& textures_;
    IpEndpoint lobbyEndpoint_;
    ThreadSafeQueue<NotificationData>& broadcastQueue_;
    const std::atomic<bool>& runningFlag_;
    std::unique_ptr<LobbyConnection> lobbyConnection_;
    LobbyConnection* sharedConnection_{nullptr};
    bool ownsConnection_{true};
    Result result_;
    State state_{State::Loading};

    std::vector<RoomInfo> rooms_;
    std::vector<EntityId> roomButtonEntities_;

    EntityId backgroundEntity_{0};
    EntityId logoEntity_{0};
    EntityId titleEntity_{0};
    EntityId statusEntity_{0};

    EntityId statsBoxEntity_{0};
    EntityId statsUsernameEntity_{0};
    EntityId statsGamesEntity_{0};
    EntityId statsWinsEntity_{0};
    EntityId statsLossesEntity_{0};
    EntityId statsWinRateEntity_{0};
    EntityId statsScoreEntity_{0};

    EntityId createButtonEntity_{0};
    EntityId refreshButtonEntity_{0};
    EntityId backButtonEntity_{0};
    EntityId filterFullButtonEntity_{0};
    EntityId filterProtectedButtonEntity_{0};

    float refreshTimer_{0.0F};
    bool statsLoaded_{false};
    int consecutiveFailures_{0};
    bool filterShowFull_{true};
    bool filterShowProtected_{true};
    bool filterChanged_{false};
    bool createMenuInitialized_{false};
    bool passwordMenuInitialized_{false};
    bool roomWaitingMenuInitialized_{false};
    constexpr static float kRefreshInterval = 1.0F;

    std::unique_ptr<CreateRoomMenu> createRoomMenu_;
    std::unique_ptr<PasswordInputMenu> passwordInputMenu_;
    std::unique_ptr<RoomWaitingMenu> roomWaitingMenu_;
    std::size_t pendingJoinRoomIndex_{0};
    bool isRoomHost_{false};
    bool isRefreshing_{false};
    bool isCreating_{false};
    bool isJoining_{false};
    bool isGettingStats_{false};
};
