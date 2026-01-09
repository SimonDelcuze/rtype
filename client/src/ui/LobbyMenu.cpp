#include "ui/LobbyMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "ui/NotificationData.hpp"

#include <sstream>

namespace
{
    EntityId createBackground(Registry& registry, TextureManager& textures)
    {
        if (!textures.has("menu_bg"))
            textures.load("menu_bg", "client/assets/backgrounds/menu.jpg");

        auto tex = textures.get("menu_bg");
        if (tex == nullptr)
            return 0;

        EntityId entity  = registry.createEntity();
        auto& transform  = registry.emplace<TransformComponent>(entity);
        transform.x      = 0.0F;
        transform.y      = 0.0F;
        transform.scaleX = 2.25F;
        transform.scaleY = 2.0F;

        SpriteComponent sprite(tex);
        registry.emplace<SpriteComponent>(entity, sprite);
        return entity;
    }

    EntityId createLogo(Registry& registry, TextureManager& textures)
    {
        if (!textures.has("logo"))
            textures.load("logo", "client/assets/other/rtype-logo.png");

        auto tex = textures.get("logo");
        if (tex == nullptr)
            return 0;

        EntityId entity  = registry.createEntity();
        auto& transform  = registry.emplace<TransformComponent>(entity);
        transform.x      = 325.0F;
        transform.y      = 0.0F;
        transform.scaleX = 2.0F;
        transform.scaleY = 2.0F;

        SpriteComponent sprite(tex);
        registry.emplace<SpriteComponent>(entity, sprite);
        return entity;
    }

    EntityId createText(Registry& registry, float x, float y, const std::string& content, unsigned int size,
                        const Color& color)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto text    = TextComponent::create("ui", size, color);
        text.content = content;
        registry.emplace<TextComponent>(entity, text);
        return entity;
    }

    EntityId createButton(Registry& registry, float x, float y, float width, float height, const std::string& label,
                          const Color& fill, std::function<void()> onClick)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto box =
            BoxComponent::create(width, height, fill,
                                 Color(static_cast<std::uint8_t>(fill.r + 40), static_cast<std::uint8_t>(fill.g + 40),
                                       static_cast<std::uint8_t>(fill.b + 40)));
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        return entity;
    }

    std::string roomStateToString(RoomState state)
    {
        switch (state) {
            case RoomState::Waiting:
                return "Waiting";
            case RoomState::Countdown:
                return "Starting...";
            case RoomState::Playing:
                return "In Game";
            case RoomState::Finished:
                return "Finished";
            default:
                return "Unknown";
        }
    }
} // namespace

LobbyMenu::LobbyMenu(FontManager& fonts, TextureManager& textures, const IpEndpoint& lobbyEndpoint,
                     ThreadSafeQueue<NotificationData>& broadcastQueue, const std::atomic<bool>& runningFlag)
    : fonts_(fonts), textures_(textures), lobbyEndpoint_(lobbyEndpoint), broadcastQueue_(broadcastQueue),
      runningFlag_(runningFlag)
{}

void LobbyMenu::create(Registry& registry)
{
    if (!fonts_.has("ui")) {
        fonts_.load("ui", "client/assets/fonts/ui.ttf");
    }

    backgroundEntity_ = createBackground(registry, textures_);
    logoEntity_       = createLogo(registry, textures_);

    titleEntity_  = createText(registry, 400.0F, 200.0F, "Game Lobby", 36, Color::White);
    statusEntity_ = createText(registry, 400.0F, 250.0F, "Connecting to lobby...", 20, Color(200, 200, 200));

    createButtonEntity_ = createButton(registry, 400.0F, 320.0F, 200.0F, 50.0F, "Create Room", Color(0, 120, 200),
                                       [this]() { onCreateRoomClicked(); });

    refreshButtonEntity_ = createButton(registry, 620.0F, 320.0F, 180.0F, 50.0F, "Refresh", Color(80, 80, 80),
                                        [this]() { onRefreshClicked(); });

    backButtonEntity_ = createButton(registry, 820.0F, 320.0F, 150.0F, 50.0F, "Back", Color(120, 50, 50),
                                     [this]() { onBackClicked(); });

    filterFullButtonEntity_ = createButton(registry, 150.0F, 320.0F, 200.0F, 50.0F, "Hide Full", Color(60, 100, 60),
                                           [this]() { onToggleFilterFull(); });

    filterProtectedButtonEntity_ = createButton(registry, 150.0F, 385.0F, 200.0F, 50.0F, "Hide Protected",
                                                Color(60, 100, 60), [this]() { onToggleFilterProtected(); });

    lobbyConnection_ = std::make_unique<LobbyConnection>(lobbyEndpoint_, runningFlag_);

    if (!lobbyConnection_->connect()) {
        Logger::instance().error("[LobbyMenu] Failed to connect to lobby server");
        if (registry.has<TextComponent>(statusEntity_)) {
            registry.get<TextComponent>(statusEntity_).content = "Failed to connect!";
        }
        state_                = State::Done;
        result_.exitRequested = true;
        return;
    }

    createRoomMenu_    = std::make_unique<CreateRoomMenu>(fonts_, textures_);
    passwordInputMenu_ = std::make_unique<PasswordInputMenu>(fonts_, textures_);

    refreshRoomList();
    updateRoomListDisplay(registry);
}

void LobbyMenu::destroy(Registry& registry)
{
    registry.clear();

    if (lobbyConnection_) {
        lobbyConnection_->disconnect();
        lobbyConnection_.reset();
    }
}

bool LobbyMenu::isDone() const
{
    return state_ == State::Done;
}

void LobbyMenu::handleEvent(Registry& registry, const Event& event)
{
    (void) registry;
    (void) event;
}

void LobbyMenu::render(Registry& registry, Window& window)
{
    if (state_ == State::ShowingCreateMenu) {
        if (createRoomMenu_) {
            if (!createMenuInitialized_) {
                for (EntityId id : roomButtonEntities_) {
                    if (registry.isAlive(id)) {
                        registry.destroyEntity(id);
                    }
                }
                roomButtonEntities_.clear();

                if (registry.isAlive(titleEntity_))
                    registry.destroyEntity(titleEntity_);
                if (registry.isAlive(statusEntity_))
                    registry.destroyEntity(statusEntity_);
                if (registry.isAlive(createButtonEntity_))
                    registry.destroyEntity(createButtonEntity_);
                if (registry.isAlive(refreshButtonEntity_))
                    registry.destroyEntity(refreshButtonEntity_);
                if (registry.isAlive(backButtonEntity_))
                    registry.destroyEntity(backButtonEntity_);
                if (registry.isAlive(filterFullButtonEntity_))
                    registry.destroyEntity(filterFullButtonEntity_);
                if (registry.isAlive(filterProtectedButtonEntity_))
                    registry.destroyEntity(filterProtectedButtonEntity_);

                createRoomMenu_->create(registry);
                createMenuInitialized_ = true;
            }

            createRoomMenu_->render(registry, window);

            if (createRoomMenu_->isDone()) {
                auto result = createRoomMenu_->getResult(registry);
                createRoomMenu_->destroy(registry);
                createMenuInitialized_ = false;

                backgroundEntity_ = createBackground(registry, textures_);
                logoEntity_       = createLogo(registry, textures_);

                titleEntity_ = createText(registry, 400.0F, 200.0F, "Game Lobby", 36, Color::White);
                statusEntity_ =
                    createText(registry, 400.0F, 250.0F, "Connecting to lobby...", 20, Color(200, 200, 200));

                createButtonEntity_ = createButton(registry, 400.0F, 320.0F, 200.0F, 50.0F, "Create Room",
                                                   Color(0, 120, 200), [this]() { onCreateRoomClicked(); });

                refreshButtonEntity_ = createButton(registry, 620.0F, 320.0F, 180.0F, 50.0F, "Refresh",
                                                    Color(80, 80, 80), [this]() { onRefreshClicked(); });

                backButtonEntity_ = createButton(registry, 820.0F, 320.0F, 150.0F, 50.0F, "Back", Color(120, 50, 50),
                                                 [this]() { onBackClicked(); });

                filterFullButtonEntity_ = createButton(registry, 150.0F, 320.0F, 200.0F, 50.0F, "Hide Full",
                                                       Color(60, 100, 60), [this]() { onToggleFilterFull(); });

                filterProtectedButtonEntity_ =
                    createButton(registry, 150.0F, 385.0F, 200.0F, 50.0F, "Hide Protected", Color(60, 100, 60),
                                 [this]() { onToggleFilterProtected(); });

                if (result.created) {
                    Logger::instance().info("[LobbyMenu] Creating room with configuration...");
                    state_ = State::Creating;

                    if (!lobbyConnection_) {
                        state_                = State::Done;
                        result_.exitRequested = true;
                        return;
                    }

                    auto createResult =
                        lobbyConnection_->createRoom(result.roomName, result.password, result.visibility);

                    if (!createResult.has_value()) {
                        Logger::instance().error("[LobbyMenu] Failed to create room with configuration");
                        state_ = State::ShowingRooms;
                        updateRoomListDisplay(registry);
                        return;
                    }

                    Logger::instance().info("[LobbyMenu] Room created: Name='" + result.roomName +
                                            "' ID=" + std::to_string(createResult->roomId) +
                                            " Port=" + std::to_string(createResult->port));

                    Logger::instance().info("[LobbyMenu] Joining own room...");
                    auto joinResult = lobbyConnection_->joinRoom(createResult->roomId);

                    if (!joinResult.has_value()) {
                        Logger::instance().error("[LobbyMenu] Failed to join own room");
                        state_ = State::ShowingRooms;
                        updateRoomListDisplay(registry);
                        return;
                    }

                    result_.roomId   = createResult->roomId;
                    result_.gamePort = createResult->port;
                    isRoomHost_      = true;
                    state_           = State::InRoom;
                } else {
                    Logger::instance().info("[LobbyMenu] Room creation cancelled");
                    state_ = State::ShowingRooms;
                    updateRoomListDisplay(registry);
                }
            }
        }
        return;
    }

    if (state_ == State::ShowingPasswordInput) {
        if (passwordInputMenu_) {
            if (!passwordMenuInitialized_) {
                for (EntityId id : roomButtonEntities_) {
                    if (registry.isAlive(id)) {
                        registry.destroyEntity(id);
                    }
                }
                roomButtonEntities_.clear();

                if (registry.isAlive(titleEntity_))
                    registry.destroyEntity(titleEntity_);
                if (registry.isAlive(statusEntity_))
                    registry.destroyEntity(statusEntity_);
                if (registry.isAlive(createButtonEntity_))
                    registry.destroyEntity(createButtonEntity_);
                if (registry.isAlive(refreshButtonEntity_))
                    registry.destroyEntity(refreshButtonEntity_);
                if (registry.isAlive(backButtonEntity_))
                    registry.destroyEntity(backButtonEntity_);
                if (registry.isAlive(filterFullButtonEntity_))
                    registry.destroyEntity(filterFullButtonEntity_);
                if (registry.isAlive(filterProtectedButtonEntity_))
                    registry.destroyEntity(filterProtectedButtonEntity_);

                passwordInputMenu_->create(registry);
                passwordMenuInitialized_ = true;
            }

            passwordInputMenu_->render(registry, window);

            if (passwordInputMenu_->isDone()) {
                auto result = passwordInputMenu_->getResult(registry);
                passwordInputMenu_->destroy(registry);
                passwordMenuInitialized_ = false;

                backgroundEntity_ = createBackground(registry, textures_);
                logoEntity_       = createLogo(registry, textures_);

                titleEntity_ = createText(registry, 400.0F, 200.0F, "Game Lobby", 36, Color::White);
                statusEntity_ =
                    createText(registry, 400.0F, 250.0F, "Connecting to lobby...", 20, Color(200, 200, 200));

                createButtonEntity_ = createButton(registry, 400.0F, 320.0F, 200.0F, 50.0F, "Create Room",
                                                   Color(0, 120, 200), [this]() { onCreateRoomClicked(); });

                refreshButtonEntity_ = createButton(registry, 620.0F, 320.0F, 180.0F, 50.0F, "Refresh",
                                                    Color(80, 80, 80), [this]() { onRefreshClicked(); });

                backButtonEntity_ = createButton(registry, 820.0F, 320.0F, 150.0F, 50.0F, "Back", Color(120, 50, 50),
                                                 [this]() { onBackClicked(); });

                filterFullButtonEntity_ = createButton(registry, 150.0F, 320.0F, 200.0F, 50.0F, "Hide Full",
                                                       Color(60, 100, 60), [this]() { onToggleFilterFull(); });

                filterProtectedButtonEntity_ =
                    createButton(registry, 150.0F, 385.0F, 200.0F, 50.0F, "Hide Protected", Color(60, 100, 60),
                                 [this]() { onToggleFilterProtected(); });

                if (result.submitted) {
                    Logger::instance().info("[LobbyMenu] Password submitted, joining room...");
                    state_ = State::Joining;

                    if (!lobbyConnection_ || pendingJoinRoomIndex_ >= rooms_.size()) {
                        state_                = State::Done;
                        result_.exitRequested = true;
                        return;
                    }

                    const auto& room = rooms_[pendingJoinRoomIndex_];
                    auto joinResult  = lobbyConnection_->joinRoom(room.roomId, result.password);

                    if (!joinResult.has_value()) {
                        Logger::instance().error("[LobbyMenu] Failed to join room with password");
                        broadcastQueue_.push(NotificationData{"Incorrect password or failed to join", 3.0F});
                        state_ = State::ShowingRooms;
                        updateRoomListDisplay(registry);
                        return;
                    }

                    Logger::instance().info(
                        "[LobbyMenu] Joined password-protected room: ID=" + std::to_string(joinResult->roomId) +
                        " Port=" + std::to_string(joinResult->port));

                    result_.roomId   = joinResult->roomId;
                    result_.gamePort = joinResult->port;
                    isRoomHost_      = false;
                    state_           = State::InRoom;
                } else {
                    Logger::instance().info("[LobbyMenu] Password input cancelled");
                    state_ = State::ShowingRooms;
                    updateRoomListDisplay(registry);
                }
            }
        }
        return;
    }

    if (state_ == State::InRoom) {
        if (!roomWaitingMenu_) {
            roomWaitingMenu_ = std::make_unique<RoomWaitingMenu>(fonts_, textures_, result_.roomId, result_.gamePort,
                                                                 isRoomHost_, lobbyConnection_.get());
        }

        if (!roomWaitingMenuInitialized_) {
            for (EntityId id : roomButtonEntities_) {
                if (registry.isAlive(id)) {
                    registry.destroyEntity(id);
                }
            }
            roomButtonEntities_.clear();

            if (registry.isAlive(titleEntity_))
                registry.destroyEntity(titleEntity_);
            if (registry.isAlive(statusEntity_))
                registry.destroyEntity(statusEntity_);
            if (registry.isAlive(createButtonEntity_))
                registry.destroyEntity(createButtonEntity_);
            if (registry.isAlive(refreshButtonEntity_))
                registry.destroyEntity(refreshButtonEntity_);
            if (registry.isAlive(backButtonEntity_))
                registry.destroyEntity(backButtonEntity_);
            if (registry.isAlive(filterFullButtonEntity_))
                registry.destroyEntity(filterFullButtonEntity_);
            if (registry.isAlive(filterProtectedButtonEntity_))
                registry.destroyEntity(filterProtectedButtonEntity_);

            roomWaitingMenu_->create(registry);
            roomWaitingMenuInitialized_ = true;
        }

        roomWaitingMenu_->render(registry, window);

        if (roomWaitingMenu_->isDone()) {
            auto result = roomWaitingMenu_->getResult(registry);
            roomWaitingMenu_->destroy(registry);
            roomWaitingMenu_.reset();
            roomWaitingMenuInitialized_ = false;

            if (result.startGame) {
                Logger::instance().info("[LobbyMenu] Starting game with " + std::to_string(result.expectedPlayerCount) +
                                        " expected players...");
                result_.success             = true;
                result_.isHost              = isRoomHost_;
                result_.expectedPlayerCount = result.expectedPlayerCount;
                state_                      = State::Done;
            } else if (result.leaveRoom) {
                Logger::instance().info("[LobbyMenu] Leaving room...");
                titleEntity_ = createText(registry, 400.0F, 200.0F, "Game Lobby", 36, Color::White);
                statusEntity_ =
                    createText(registry, 400.0F, 250.0F, "Connecting to lobby...", 20, Color(200, 200, 200));

                createButtonEntity_ = createButton(registry, 400.0F, 320.0F, 200.0F, 50.0F, "Create Room",
                                                   Color(0, 120, 200), [this]() { onCreateRoomClicked(); });

                refreshButtonEntity_ = createButton(registry, 620.0F, 320.0F, 180.0F, 50.0F, "Refresh",
                                                    Color(80, 80, 80), [this]() { onRefreshClicked(); });

                backButtonEntity_ = createButton(registry, 820.0F, 320.0F, 150.0F, 50.0F, "Back", Color(120, 50, 50),
                                                 [this]() { onBackClicked(); });

                filterFullButtonEntity_ = createButton(registry, 150.0F, 320.0F, 200.0F, 50.0F, "Hide Full",
                                                       Color(60, 100, 60), [this]() { onToggleFilterFull(); });

                filterProtectedButtonEntity_ =
                    createButton(registry, 150.0F, 385.0F, 200.0F, 50.0F, "Hide Protected", Color(60, 100, 60),
                                 [this]() { onToggleFilterProtected(); });

                state_ = State::ShowingRooms;
                updateRoomListDisplay(registry);
            }
        }
        return;
    }

    if (lobbyConnection_) {
        lobbyConnection_->poll(broadcastQueue_);
        if (lobbyConnection_->isServerLost()) {
            Logger::instance().warn("[LobbyMenu] Server lost - returning to connection menu");
            broadcastQueue_.push(NotificationData{"Lost connection to lobby server", 5.0F});
            state_                = State::Done;
            result_.backRequested = true;
            return;
        }
    }

    if (filterChanged_) {
        filterChanged_ = false;
        updateRoomListDisplay(registry);
    }

    if (state_ == State::Loading || state_ == State::ShowingRooms) {
        refreshTimer_ += 0.016F;
        if (refreshTimer_ >= kRefreshInterval) {
            refreshTimer_ = 0.0F;
            refreshRoomList();
            updateRoomListDisplay(registry);
        }
    }
}

void LobbyMenu::refreshRoomList()
{
    if (!lobbyConnection_) {
        return;
    }

    auto result = lobbyConnection_->requestRoomList();

    if (!result.has_value()) {
        Logger::instance().warn("[LobbyMenu] Failed to get room list");
        consecutiveFailures_++;
        if (consecutiveFailures_ >= 3) {
            Logger::instance().error("[LobbyMenu] Connection to lobby server lost (3 timeouts)");
            broadcastQueue_.push(NotificationData{"Server disconnected", 5.0F});
            state_                = State::Done;
            result_.backRequested = true;
        }
        return;
    }

    consecutiveFailures_ = 0;

    rooms_ = result->rooms;

    if (state_ == State::Loading) {
        state_ = State::ShowingRooms;
    }

    Logger::instance().info("[LobbyMenu] Received " + std::to_string(rooms_.size()) + " rooms");
}

void LobbyMenu::onCreateRoomClicked()
{
    Logger::instance().info("[LobbyMenu] Opening create room menu...");
    state_ = State::ShowingCreateMenu;
}

void LobbyMenu::onJoinRoomClicked(std::size_t roomIndex)
{
    if (roomIndex >= rooms_.size()) {
        return;
    }

    const auto& room = rooms_[roomIndex];

    if (room.passwordProtected) {
        Logger::instance().info("[LobbyMenu] Room " + std::to_string(room.roomId) +
                                " is password-protected, showing password input...");
        pendingJoinRoomIndex_ = roomIndex;
        state_                = State::ShowingPasswordInput;
        return;
    }

    Logger::instance().info("[LobbyMenu] Joining room " + std::to_string(room.roomId) + "...");
    state_ = State::Joining;

    if (!lobbyConnection_) {
        state_                = State::Done;
        result_.exitRequested = true;
        return;
    }

    auto result = lobbyConnection_->joinRoom(room.roomId);

    if (!result.has_value()) {
        Logger::instance().error("[LobbyMenu] Failed to join room");
        state_ = State::ShowingRooms;
        return;
    }

    Logger::instance().info("[LobbyMenu] Joined room: ID=" + std::to_string(result->roomId) +
                            " Port=" + std::to_string(result->port));

    result_.roomId   = result->roomId;
    result_.gamePort = result->port;
    isRoomHost_      = false;
    state_           = State::InRoom;
}

void LobbyMenu::onRefreshClicked()
{
    Logger::instance().info("[LobbyMenu] Refreshing room list...");
    refreshRoomList();
}

void LobbyMenu::onBackClicked()
{
    Logger::instance().info("[LobbyMenu] Back button clicked");
    result_.backRequested = true;
    state_                = State::Done;
}

void LobbyMenu::onToggleFilterFull()
{
    filterShowFull_ = !filterShowFull_;
    filterChanged_  = true;
    Logger::instance().info("[LobbyMenu] Filter full rooms: " + std::string(filterShowFull_ ? "SHOW" : "HIDE"));
}

void LobbyMenu::onToggleFilterProtected()
{
    filterShowProtected_ = !filterShowProtected_;
    filterChanged_       = true;
    Logger::instance().info("[LobbyMenu] Filter protected rooms: " +
                            std::string(filterShowProtected_ ? "SHOW" : "HIDE"));
}

bool LobbyMenu::shouldShowRoom(const RoomInfo& room) const
{
    if (!filterShowFull_ && room.playerCount >= room.maxPlayers) {
        return false;
    }
    if (!filterShowProtected_ && room.passwordProtected) {
        return false;
    }
    return true;
}

void LobbyMenu::updateRoomListDisplay(Registry& registry)
{
    for (EntityId id : roomButtonEntities_) {
        if (registry.isAlive(id)) {
            registry.destroyEntity(id);
        }
    }
    roomButtonEntities_.clear();

    std::size_t displayIndex = 0;
    std::size_t visibleCount = 0;
    for (std::size_t i = 0; i < rooms_.size(); ++i) {
        if (shouldShowRoom(rooms_[i])) {
            createRoomButton(registry, rooms_[i], displayIndex);
            displayIndex++;
            visibleCount++;
        }
    }

    if (registry.has<TextComponent>(statusEntity_)) {
        if (rooms_.empty()) {
            registry.get<TextComponent>(statusEntity_).content = "No rooms available. Create one!";
        } else if (visibleCount == 0) {
            registry.get<TextComponent>(statusEntity_).content =
                "No rooms match filters. (" + std::to_string(rooms_.size()) + " total)";
        } else {
            registry.get<TextComponent>(statusEntity_).content =
                "Showing " + std::to_string(visibleCount) + " / " + std::to_string(rooms_.size()) + " room(s)";
        }
    }
}

void LobbyMenu::createRoomButton(Registry& registry, const RoomInfo& room, std::size_t index)
{
    float startY  = 400.0F;
    float spacing = 70.0F;
    float y       = startY + static_cast<float>(index) * spacing;

    std::ostringstream label;
    if (room.passwordProtected) {
        label << "[LOCK] ";
    }
    label << room.roomName << " [" << room.playerCount << "/" << room.maxPlayers << "] - "
          << roomStateToString(room.state);

    Color buttonColor = Color(60, 80, 120);
    if (room.state == RoomState::Playing || room.state == RoomState::Finished) {
        buttonColor = Color(80, 80, 80);
    } else if (room.playerCount >= room.maxPlayers) {
        buttonColor = Color(100, 60, 60);
    }

    auto buttonEntity = createButton(registry, 400.0F, y, 600.0F, 50.0F, label.str(), buttonColor,
                                     [this, index]() { onJoinRoomClicked(index); });

    roomButtonEntities_.push_back(buttonEntity);
}
