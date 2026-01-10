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

#include <iomanip>
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
                     ThreadSafeQueue<NotificationData>& broadcastQueue, const std::atomic<bool>& runningFlag,
                     LobbyConnection* sharedConnection)
    : fonts_(fonts), textures_(textures), lobbyEndpoint_(lobbyEndpoint), broadcastQueue_(broadcastQueue),
      runningFlag_(runningFlag), sharedConnection_(sharedConnection), ownsConnection_(sharedConnection == nullptr)
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

    statsBoxEntity_         = registry.createEntity();
    auto& statsBoxTransform = registry.emplace<TransformComponent>(statsBoxEntity_);
    statsBoxTransform.x     = 20.0F;
    statsBoxTransform.y     = 20.0F;
    auto statsBox           = BoxComponent::create(280.0F, 180.0F, Color(30, 35, 45, 220), Color(50, 55, 65, 220));
    registry.emplace<BoxComponent>(statsBoxEntity_, statsBox);

    statsUsernameEntity_ = createText(registry, 30.0F, 30.0F, "", 18, Color(150, 200, 255));
    statsGamesEntity_    = createText(registry, 30.0F, 60.0F, "", 14, Color(200, 200, 200));
    statsWinsEntity_     = createText(registry, 30.0F, 85.0F, "", 14, Color(100, 255, 100));
    statsLossesEntity_   = createText(registry, 30.0F, 110.0F, "", 14, Color(255, 100, 100));
    statsWinRateEntity_  = createText(registry, 30.0F, 135.0F, "", 14, Color(255, 200, 100));
    statsScoreEntity_    = createText(registry, 30.0F, 160.0F, "", 14, Color(255, 255, 100));

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

    if (sharedConnection_) {
        ownsConnection_ = false;
    } else {
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
        ownsConnection_ = true;
    }

    createRoomMenu_    = std::make_unique<CreateRoomMenu>(fonts_, textures_);
    passwordInputMenu_ = std::make_unique<PasswordInputMenu>(fonts_, textures_);

    refreshRoomList();
    updateRoomListDisplay(registry);
}

void LobbyMenu::destroy(Registry& registry)
{
    registry.clear();

    if (ownsConnection_ && lobbyConnection_) {
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

                    auto* conn = getConnection();
                    if (!conn) {
                        state_                = State::Done;
                        result_.exitRequested = true;
                        return;
                    }

                    conn->sendCreateRoom(result.roomName, result.password, result.visibility);
                    state_ = State::Creating;
                    isCreating_ = true;

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
                for (EntityId id : roomButtonEntities_) { if (registry.isAlive(id)) registry.destroyEntity(id); }
                roomButtonEntities_.clear();
                if (registry.isAlive(titleEntity_)) registry.destroyEntity(titleEntity_);
                if (registry.isAlive(statusEntity_)) registry.destroyEntity(statusEntity_);
                if (registry.isAlive(createButtonEntity_)) registry.destroyEntity(createButtonEntity_);
                if (registry.isAlive(refreshButtonEntity_)) registry.destroyEntity(refreshButtonEntity_);
                if (registry.isAlive(backButtonEntity_)) registry.destroyEntity(backButtonEntity_);
                if (registry.isAlive(filterFullButtonEntity_)) registry.destroyEntity(filterFullButtonEntity_);
                if (registry.isAlive(filterProtectedButtonEntity_)) registry.destroyEntity(filterProtectedButtonEntity_);

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
                statusEntity_ = createText(registry, 400.0F, 250.0F, "Connecting to lobby...", 20, Color(200, 200, 200));
                createButtonEntity_ = createButton(registry, 400.0F, 320.0F, 200.0F, 50.0F, "Create Room", Color(0, 120, 200), [this]() { onCreateRoomClicked(); });
                refreshButtonEntity_ = createButton(registry, 620.0F, 320.0F, 180.0F, 50.0F, "Refresh", Color(80, 80, 80), [this]() { onRefreshClicked(); });
                backButtonEntity_ = createButton(registry, 820.0F, 320.0F, 150.0F, 50.0F, "Back", Color(120, 50, 50), [this]() { onBackClicked(); });
                filterFullButtonEntity_ = createButton(registry, 150.0F, 320.0F, 200.0F, 50.0F, "Hide Full", Color(60, 100, 60), [this]() { onToggleFilterFull(); });
                filterProtectedButtonEntity_ = createButton(registry, 150.0F, 385.0F, 200.0F, 50.0F, "Hide Protected", Color(60, 100, 60), [this]() { onToggleFilterProtected(); });

                if (result.submitted) {
                    Logger::instance().info("[LobbyMenu] Password submitted, joining room...");

                    auto* conn = getConnection();
                    if (!conn || pendingJoinRoomIndex_ >= rooms_.size()) {
                        state_                = State::Done;
                        result_.exitRequested = true;
                        return;
                    }

                    const auto& room = rooms_[pendingJoinRoomIndex_];
                    conn->sendJoinRoom(room.roomId, result.password);
                    state_ = State::Joining;
                    isJoining_ = true;
                    isRoomHost_ = false;
                } else {
                    Logger::instance().info("[LobbyMenu] Password input cancelled");
                    state_ = State::ShowingRooms;
                    updateRoomListDisplay(registry);
                }
            }
        }
        return;
    }

    if (state_ == State::InRoom && roomWaitingMenu_) {
        roomWaitingMenu_->render(registry, window);
        return;
    }
}

void LobbyMenu::update(Registry& registry, float dt)
{
    auto* conn = getConnection();
    if (conn) {
        conn->poll(broadcastQueue_);
        if (conn->isServerLost() && !result_.serverLost) {
            Logger::instance().warn("[LobbyMenu] Server lost - returning to connection menu");
            broadcastQueue_.push(NotificationData{"Lost connection to lobby server", 5.0F});
            state_                = State::Done;
            result_.serverLost    = true;
            result_.backRequested = true;
            return;
        }

        if (isRefreshing_) {
            if (conn->hasRoomListResult()) {
                auto result = conn->popRoomListResult();
                isRefreshing_ = false;
                if (result.has_value()) {
                     rooms_ = result->rooms;
                     if (state_ == State::Loading) state_ = State::ShowingRooms;
                     Logger::instance().info("[LobbyMenu] Received " + std::to_string(rooms_.size()) + " rooms");
                     updateRoomListDisplay(registry);
                } else {
                     Logger::instance().warn("[LobbyMenu] Failed to refresh rooms");
                }
            }
        }

        if (isCreating_) {
            if (conn->hasRoomCreatedResult()) {
                auto result = conn->popRoomCreatedResult();
                isCreating_ = false;
                if (result.has_value()) {
                    Logger::instance().info("[LobbyMenu] Room created: ID=" + std::to_string(result->roomId));
                    Logger::instance().info("[LobbyMenu] Joining own room...");
                    conn->sendJoinRoom(result->roomId);
                    isJoining_ = true;
                    isRoomHost_ = true;
                } else {
                    Logger::instance().error("[LobbyMenu] Failed to create room");
                    state_ = State::ShowingRooms;
                    broadcastQueue_.push(NotificationData{"Failed to create room", 3.0F});
                    updateRoomListDisplay(registry);
                }
            }
        }

        if (isJoining_) {
            if (conn->hasJoinRoomResult()) {
                auto result = conn->popJoinRoomResult();
                isJoining_ = false;
                if (result.has_value()) {
                     Logger::instance().info("[LobbyMenu] Joined room: ID=" + std::to_string(result->roomId));
                     result_.roomId = result->roomId;
                     result_.gamePort = result->port;
                     state_ = State::InRoom;
                } else {
                     Logger::instance().error("[LobbyMenu] Failed to join room");
                     broadcastQueue_.push(NotificationData{"Failed to join room", 3.0F});
                     state_ = State::ShowingRooms;
                     updateRoomListDisplay(registry);
                }
            }
        }
    }

    if (state_ == State::InRoom) {
        if (!roomWaitingMenu_) {
            roomWaitingMenu_ = std::make_unique<RoomWaitingMenu>(fonts_, textures_, result_.roomId, result_.gamePort,
                                                                 isRoomHost_, getConnection());
        }

        if (!roomWaitingMenuInitialized_) {
            for (EntityId id : roomButtonEntities_) { if (registry.isAlive(id)) registry.destroyEntity(id); }
            roomButtonEntities_.clear();
            if (registry.isAlive(titleEntity_)) registry.destroyEntity(titleEntity_);
            if (registry.isAlive(statusEntity_)) registry.destroyEntity(statusEntity_);
            if (registry.isAlive(createButtonEntity_)) registry.destroyEntity(createButtonEntity_);
            if (registry.isAlive(refreshButtonEntity_)) registry.destroyEntity(refreshButtonEntity_);
            if (registry.isAlive(backButtonEntity_)) registry.destroyEntity(backButtonEntity_);
            if (registry.isAlive(filterFullButtonEntity_)) registry.destroyEntity(filterFullButtonEntity_);
            if (registry.isAlive(filterProtectedButtonEntity_)) registry.destroyEntity(filterProtectedButtonEntity_);

            roomWaitingMenu_->create(registry);
            roomWaitingMenuInitialized_ = true;
        }

        roomWaitingMenu_->update(registry, dt);

        if (roomWaitingMenu_->isDone()) {
            auto result = roomWaitingMenu_->getResult(registry);
            roomWaitingMenu_->destroy(registry);
            roomWaitingMenu_.reset();
            roomWaitingMenuInitialized_ = false;

            if (result.startGame) {
                result_.success = true;
                result_.isHost = isRoomHost_;
                result_.expectedPlayerCount = result.expectedPlayerCount;
                state_ = State::Done;
            } else if (result.serverLost) {
                 state_ = State::Done;
                 result_.serverLost = true;
                 result_.backRequested = true;
            } else if (result.leaveRoom) {
                 titleEntity_ = createText(registry, 400.0F, 200.0F, "Game Lobby", 36, Color::White);
                 statusEntity_ = createText(registry, 400.0F, 250.0F, "Connecting to lobby...", 20, Color(200, 200, 200));
                 createButtonEntity_ = createButton(registry, 400.0F, 320.0F, 200.0F, 50.0F, "Create Room", Color(0, 120, 200), [this](){ onCreateRoomClicked(); });
                 refreshButtonEntity_ = createButton(registry, 620.0F, 320.0F, 180.0F, 50.0F, "Refresh", Color(80, 80, 80), [this](){ onRefreshClicked(); });
                 backButtonEntity_ = createButton(registry, 820.0F, 320.0F, 150.0F, 50.0F, "Back", Color(120, 50, 50), [this](){ onBackClicked(); });
                 filterFullButtonEntity_ = createButton(registry, 150.0F, 320.0F, 200.0F, 50.0F, "Hide Full", Color(60, 100, 60), [this](){ onToggleFilterFull(); });
                 filterProtectedButtonEntity_ = createButton(registry, 150.0F, 385.0F, 200.0F, 50.0F, "Hide Protected", Color(60, 100, 60), [this](){ onToggleFilterProtected(); });

                 state_ = State::ShowingRooms;
                 refreshRoomList();
            }
        }
        return;
    }

    if (filterChanged_) {
        filterChanged_ = false;
        updateRoomListDisplay(registry);
    }

    if (!statsLoaded_ && sharedConnection_) {
        Logger::instance().info("[LobbyMenu] Loading stats with authenticated connection");
        loadAndDisplayStats(registry);
        statsLoaded_ = true;
    }

    refreshTimer_ += dt;
    if (refreshTimer_ >= kRefreshInterval) {
        refreshTimer_ = 0.0F;
        if (state_ == State::ShowingRooms || state_ == State::Loading) {
             if (!isRefreshing_) refreshRoomList();
        } else {
        }
    }
}

void LobbyMenu::refreshRoomList()
{
    auto* conn = getConnection();
    if (!conn) return;

    if (isRefreshing_) return;

    conn->sendRequestRoomList();
    isRefreshing_ = true;
}

void LobbyMenu::onCreateRoomClicked()
{
    Logger::instance().info("[LobbyMenu] Opening create room menu...");
    state_ = State::ShowingCreateMenu;
}

void LobbyMenu::onJoinRoomClicked(std::size_t roomIndex)
{
    if (roomIndex >= rooms_.size()) return;

    const auto& room = rooms_[roomIndex];

    if (room.passwordProtected) {
        Logger::instance().info("[LobbyMenu] Room " + std::to_string(room.roomId) + " is password-protected...");
        pendingJoinRoomIndex_ = roomIndex;
        state_                = State::ShowingPasswordInput;
        return;
    }

    if (isJoining_) return;

    Logger::instance().info("[LobbyMenu] Joining room " + std::to_string(room.roomId) + "...");
    auto* conn = getConnection();
    if (!conn) {
        state_ = State::Done;
        result_.exitRequested = true;
        return;
    }

    conn->sendJoinRoom(room.roomId);
    isJoining_ = true;
    isRoomHost_ = false;
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

void LobbyMenu::loadAndDisplayStats(Registry& registry)
{
    auto* conn = getConnection();
    if (!conn) {
        Logger::instance().warn("[LobbyMenu] Cannot load stats: no lobby connection");
        return;
    }

    Logger::instance().info("[LobbyMenu] Loading user stats...");

    auto stats = conn->getStats();
    if (!stats.has_value()) {
        Logger::instance().warn("[LobbyMenu] Failed to load user stats");
        if (registry.has<TextComponent>(statsUsernameEntity_)) {
            registry.get<TextComponent>(statsUsernameEntity_).content = "Stats unavailable";
        }
        return;
    }

    Logger::instance().info("[LobbyMenu] Received stats - userId: " + std::to_string(stats->userId) +
                            ", games: " + std::to_string(stats->gamesPlayed) +
                            ", wins: " + std::to_string(stats->wins) + ", losses: " + std::to_string(stats->losses) +
                            ", score: " + std::to_string(stats->totalScore));

    float winRate = 0.0F;
    if (stats->gamesPlayed > 0) {
        winRate = (static_cast<float>(stats->wins) / static_cast<float>(stats->gamesPlayed)) * 100.0F;
    }

    Logger::instance().info("[LobbyMenu] Calculated win rate: " + std::to_string(winRate) + "%");

    std::string username = "Player #" + std::to_string(stats->userId);

    if (registry.has<TextComponent>(statsUsernameEntity_)) {
        registry.get<TextComponent>(statsUsernameEntity_).content = username;
    }

    if (registry.has<TextComponent>(statsGamesEntity_)) {
        registry.get<TextComponent>(statsGamesEntity_).content = "Games: " + std::to_string(stats->gamesPlayed);
    }

    if (registry.has<TextComponent>(statsWinsEntity_)) {
        registry.get<TextComponent>(statsWinsEntity_).content = "Wins: " + std::to_string(stats->wins);
    }

    if (registry.has<TextComponent>(statsLossesEntity_)) {
        registry.get<TextComponent>(statsLossesEntity_).content = "Losses: " + std::to_string(stats->losses);
    }

    if (registry.has<TextComponent>(statsWinRateEntity_)) {
        std::ostringstream oss;
        oss << "Win Rate: " << std::fixed << std::setprecision(1) << winRate << "%";
        registry.get<TextComponent>(statsWinRateEntity_).content = oss.str();
    }

    if (registry.has<TextComponent>(statsScoreEntity_)) {
        registry.get<TextComponent>(statsScoreEntity_).content = "Score: " + std::to_string(stats->totalScore);
    }

    Logger::instance().info("[LobbyMenu] Stats loaded and displayed in stats box");
}
