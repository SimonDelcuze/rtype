# LobbyMenu

## Overview

The `LobbyMenu` is the central multiplayer hub where players browse available game rooms, create new rooms, join existing ones, and manage their lobby presence. It displays a live-updating list of rooms with filters, player statistics, and provides navigation to room creation and joining flows.

## Class Structure

```cpp
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

    LobbyMenu(FontManager& fonts, TextureManager& textures, 
              const IpEndpoint& lobbyEndpoint,
              ThreadSafeQueue<NotificationData>& broadcastQueue, 
              const std::atomic<bool>& runningFlag,
              LobbyConnection* sharedConnection = nullptr);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void update(Registry& registry, float dt) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry& registry) const;

  private:
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

    LobbyConnection* getConnection();

    FontManager& fonts_;
    TextureManager& textures_;
    ThreadSafeQueue<NotificationData>& broadcastQueue_;
    std::unique_ptr<LobbyConnection> lobbyConnection_;
    LobbyConnection* sharedConnection_{nullptr};
    
    Result result_;
    State state_{State::Loading};

    std::vector<RoomInfo> rooms_;
    std::vector<EntityId> roomButtonEntities_;

    float refreshTimer_{0.0F};
    bool filterShowFull_{true};
    bool filterShowProtected_{true};
    
    std::unique_ptr<CreateRoomMenu> createRoomMenu_;
    std::unique_ptr<PasswordInputMenu> passwordInputMenu_;
    std::unique_ptr<RoomWaitingMenu> roomWaitingMenu_;
};
```

## UI Layout

```
┌─────────────────────────────────────────────────────────────┐
│  R-TYPE Multiplayer Lobby                                   │
│                                                              │
│  Player: PlayerName123        Games: 42  W/L: 28/14  60%    │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ 🎮 Available Rooms                                     │ │
│  ├────────────────────────────────────────────────────────┤ │
│  │  [ Filters: ☑ Show Full  ☑ Show Protected ]           │ │
│  ├────────────────────────────────────────────────────────┤ │
│  │  Room Name           Players    Host        Status     │ │
│  │  ┌───────────────────────────────────────────────────┐│ │
│  │  │ Pro Players Only  2/4       🔒 xXProXx   [Join]   ││ │
│  │  └───────────────────────────────────────────────────┘│ │
│  │  ┌───────────────────────────────────────────────────┐│ │
│  │  │ Casual Fun        3/4          Bob123    [Join]   ││ │
│  │  └───────────────────────────────────────────────────┘│ │
│  │  ┌───────────────────────────────────────────────────┐│ │
│  │  │ FULL ROOM         4/4          Player1   [Full]   ││ │
│  │  └───────────────────────────────────────────────────┘│ │
│  └────────────────────────────────────────────────────────┘ │
│                                                              │
│  [ Create Room ]    [ Refresh ]           [ Back ]          │
└─────────────────────────────────────────────────────────────┘
```

## Room List Management

```cpp
void LobbyMenu::refreshRoomList()
{
    if (isRefreshing_) return;
    
    isRefreshing_ = true;
    
    // Request room list from server
    if (!getConnection()->sendListRoomsRequest()) {
        isRefreshing_ = false;
        broadcastQueue_.enqueue(NotificationData{
            "Failed to request room list",
            NotificationType::Error,
            3.0F
        });
        return;
    }
}

void LobbyMenu::updateRoomListDisplay(Registry& registry)
{
    // Clear old room buttons
    for (auto entity : roomButtonEntities_) {
        if (registry.entityExists(entity)) {
            registry.destroyEntity(entity);
        }
    }
    roomButtonEntities_.clear();
    
    // Create new buttons for visible rooms
    float yOffset = 220;
    std::size_t visibleIndex = 0;
    
    for (std::size_t i = 0; i < rooms_.size(); ++i) {
        const auto& room = rooms_[i];
        
        if (!shouldShowRoom(room)) continue;
        
        createRoomButton(registry, room, i);
        yOffset += 60;
        visibleIndex++;
        
        if (visibleIndex >= 6) break;  // Max 6 visible rooms
    }
}

bool LobbyMenu::shouldShowRoom(const RoomInfo& room) const
{
    // Filter full rooms if needed
    if (!filterShowFull_ && room.currentPlayers >= room.maxPlayers) {
        return false;
    }
    
    // Filter protected rooms if needed
    if (!filterShowProtected_ && room.hasPassword) {
        return false;
    }
    
    return true;
}
```

## Room Button Creation

```cpp
void LobbyMenu::createRoomButton(Registry& registry, const RoomInfo& room, 
                                  std::size_t index)
{
    const float x = 100;
    const float y = 220 + index * 60;
    const float width = 600;
    const float height = 50;
    
    // Background box
    auto bg = registry.createEntity();
    registry.addComponent(bg, TransformComponent::create(x, y));
    
    Color bgColor = room.currentPlayers >= room.maxPlayers ? 
        Color{60, 40, 40} : Color{40, 50, 60};
    registry.addComponent(bg, BoxComponent::create(width, height, 
        bgColor, Color{80, 90, 100}));
    registry.addComponent(bg, LayerComponent::create(RenderLayer::UI));
    roomButtonEntities_.push_back(bg);
    
    // Room name
    auto nameText = registry.createEntity();
    registry.addComponent(nameText, TransformComponent::create(x + 10, y + 15));
    std::string displayName = room.hasPassword ? "🔒 " + room.name : room.name;
    registry.addComponent(nameText, TextComponent::create(displayName, 16));
    registry.addComponent(nameText, LayerComponent::create(RenderLayer::UI));
    roomButtonEntities_.push_back(nameText);
    
    // Player count
    auto playersText = registry.createEntity();
    registry.addComponent(playersText, TransformComponent::create(x + 300, y + 15));
    std::string playerStr = std::to_string(room.currentPlayers) + "/" + 
                           std::to_string(room.maxPlayers);
    registry.addComponent(playersText, TextComponent::create(playerStr, 14));
    registry.addComponent(playersText, LayerComponent::create(RenderLayer::UI));
    roomButtonEntities_.push_back(playersText);
    
    // Host name
    auto hostText = registry.createEntity();
    registry.addComponent(hostText, TransformComponent::create(x + 380, y + 15));
    registry.addComponent(hostText, TextComponent::create(room.hostName, 14));
    registry.addComponent(hostText, LayerComponent::create(RenderLayer::UI));
    roomButtonEntities_.push_back(hostText);
    
    // Join button
    auto joinBtn = registry.createEntity();
    registry.addComponent(joinBtn, TransformComponent::create(x + 520, y + 5));
    
    bool isFull = room.currentPlayers >= room.maxPlayers;
    Color btnColor = isFull ? Color{60, 60, 70} : Color{50, 100, 180};
    std::string btnText = isFull ? "Full" : "Join";
    
    registry.addComponent(joinBtn, BoxComponent::create(70, 40, btnColor, Color{80, 140, 220}));
    registry.addComponent(joinBtn, ButtonComponent::create(btnText, [this, index, isFull]() {
        if (!isFull) onJoinRoomClicked(index);
    }));
    registry.addComponent(joinBtn, TextComponent::create(btnText, 14));
    registry.addComponent(joinBtn, LayerComponent::create(RenderLayer::UI));
    roomButtonEntities_.push_back(joinBtn);
}
```

## Join Room Flow

```cpp
void LobbyMenu::onJoinRoomClicked(std::size_t roomIndex)
{
    if (roomIndex >= rooms_.size()) return;
    
    const auto& room = rooms_[roomIndex];
    
    // Check if room requires password
    if (room.hasPassword) {
        // Show password input dialog
        state_ = State::ShowingPasswordInput;
        pendingJoinRoomIndex_ = roomIndex;
        
        if (!passwordInputMenu_) {
            passwordInputMenu_ = std::make_unique<PasswordInputMenu>(
                fonts_, textures_
            );
        }
        passwordInputMenu_->create(registry_);
        return;
    }
    
    // Join directly
    state_ = State::Joining;
    isJoining_ = true;
    
    JoinRoomRequest request;
    request.roomId = room.roomId;
    request.password = "";
    
    if (!getConnection()->sendJoinRoomRequest(request)) {
        state_ = State::ShowingRooms;
        isJoining_ = false;
        
        broadcastQueue_.enqueue(NotificationData{
            "Failed to send join request",
            NotificationType::Error,
            3.0F
        });
    }
}
```

## Update Loop

```cpp
void LobbyMenu::update(Registry& registry, float dt)
{
    switch (state_) {
        case State::Loading:
            // Wait for initial room list
            if (tryReceiveRoomList()) {
                state_ = State::ShowingRooms;
                updateRoomListDisplay(registry);
            }
            break;
            
        case State::ShowingRooms:
            // Auto-refresh room list
            refreshTimer_ += dt;
            if (refreshTimer_ >= kRefreshInterval) {
                refreshRoomList();
                refreshTimer_ = 0.0F;
            }
            
            // Check for room list updates
            if (tryReceiveRoomList()) {
                updateRoomListDisplay(registry);
            }
            break;
            
        case State::Creating:
            // Wait for room creation response
            if (tryReceiveCreateRoomResponse()) {
                state_ = State::InRoom;
                result_.isHost = true;
            }
            break;
            
        case State::Joining:
            // Wait for join room response
            if (tryReceiveJoinRoomResponse()) {
                state_ = State::InRoom;
                result_.isHost = false;
            }
            break;
            
        case State::InRoom:
            // In room waiting menu
            roomWaitingMenu_->update(registry, dt);
            if (roomWaitingMenu_->isDone()) {
                auto roomResult = roomWaitingMenu_->getResult();
                if (roomResult.startGame) {
                    result_.success = true;
                    done_ = true;
                }
            }
            break;
    }
}
```

## Player Statistics Display

```cpp
void LobbyMenu::loadAndDisplayStats(Registry& registry)
{
    // Request stats from server
    getConnection()->sendGetStatsRequest();
    
    // Display when received
    PlayerStats stats;
    if (getConnection()->tryGetStatsResponse(stats)) {
        statsLoaded_ = true;
        
        // Display stats in header
        auto usernameText = registry.createEntity();
        registry.addComponent(usernameText, TransformComponent::create(100, 50));
        registry.addComponent(usernameText, 
            TextComponent::create("Player: " + stats.username, 16));
        registry.addComponent(usernameText, LayerComponent::create(RenderLayer::UI));
        statsUsernameEntity_ = usernameText;
        
        auto gamesText = registry.createEntity();
        registry.addComponent(gamesText, TransformComponent::create(350, 50));
        registry.addComponent(gamesText, 
            TextComponent::create("Games: " + std::to_string(stats.gamesPlayed), 14));
        registry.addComponent(gamesText, LayerComponent::create(RenderLayer::UI));
        statsGamesEntity_ = gamesText;
        
        float winRate = stats.gamesPlayed > 0 ? 
            (float)stats.wins / stats.gamesPlayed * 100.0F : 0.0F;
        
        auto winRateText = registry.createEntity();
        registry.addComponent(winRateText, TransformComponent::create(500, 50));
        registry.addComponent(winRateText, 
            TextComponent::create("W/L: " + std::to_string(stats.wins) + "/" + 
                                 std::to_string(stats.losses) + " " +
                                 std::to_string((int)winRate) + "%", 14));
        registry.addComponent(winRateText, LayerComponent::create(RenderLayer::UI));
        statsWinRateEntity_ = winRateText;
    }
}
```

## Room Filters

```cpp
void LobbyMenu::onToggleFilterFull()
{
    filterShowFull_ = !filterShowFull_;
    filterChanged_ = true;
    updateRoomListDisplay(registry_);
}

void LobbyMenu::onToggleFilterProtected()
{
    filterShowProtected_ = !filterShowProtected_;
    filterChanged_ = true;
    updateRoomListDisplay(registry_);
}
```

## Related Menus

- [LoginMenu](login-menu.md) - Previous menu
- [CreateRoomMenu](create-room-menu.md) - Sub-menu for creating rooms
- [PasswordInputMenu](password-input-menu.md) - Sub-menu for protected rooms
- [RoomWaitingMenu](room-waiting-menu.md) - Next menu when in room

## Related Components

- [ButtonComponent](../components/button-component.md)
- [BoxComponent](../components/box-component.md)
- [TextComponent](../components/text-component.md)

## Related Systems

- [ButtonSystem](../systems/button-system.md)
- [RenderSystem](../systems/render-system.md)
- [NotificationSystem](../systems/notification-system.md)
