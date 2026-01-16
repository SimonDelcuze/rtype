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

#include <algorithm>
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
                     RoomType targetRoomType, LobbyConnection* sharedConnection)
    : fonts_(fonts), textures_(textures), lobbyEndpoint_(lobbyEndpoint), targetRoomType_(targetRoomType),
      broadcastQueue_(broadcastQueue), runningFlag_(runningFlag), sharedConnection_(sharedConnection),
      ownsConnection_(sharedConnection == nullptr)
{}

void LobbyMenu::create(Registry& registry)
{
    if (!fonts_.has("ui")) {
        fonts_.load("ui", "client/assets/fonts/ui.ttf");
    }

    backgroundEntity_ = createBackground(registry, textures_);
    logoEntity_       = createLogo(registry, textures_);

    std::string lobbyTitle = targetRoomType_ == RoomType::Ranked ? "Ranked Lobby" : "Game Lobby";
    titleEntity_           = createText(registry, 400.0F, 200.0F, lobbyTitle, 36, Color::White);
    statusEntity_          = createText(registry, 400.0F, 250.0F, "Connecting to lobby...", 20, Color(200, 200, 200));

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

    if (targetRoomType_ == RoomType::Quickplay) {
        createButtonEntity_ = createButton(registry, 400.0F, 320.0F, 200.0F, 50.0F, "Create Room", Color(0, 120, 200),
                                           [this]() { onCreateRoomClicked(); });
    }

    refreshButtonEntity_ = createButton(registry, 620.0F, 320.0F, 180.0F, 50.0F, "Refresh", Color(80, 80, 80),
                                        [this]() { onRefreshClicked(); });

    backButtonEntity_ = createButton(registry, 820.0F, 320.0F, 150.0F, 50.0F, "Back", Color(120, 50, 50),
                                     [this]() { onBackClicked(); });

    if (targetRoomType_ == RoomType::Quickplay) {
        filterFullButtonEntity_ = createButton(registry, 150.0F, 320.0F, 200.0F, 50.0F, "Hide Full", Color(60, 100, 60),
                                               [this]() { onToggleFilterFull(); });

        filterProtectedButtonEntity_ = createButton(registry, 150.0F, 385.0F, 200.0F, 50.0F, "Hide Protected",
                                                    Color(60, 100, 60), [this]() { onToggleFilterProtected(); });
    }

    spectatorCheckboxEntity_ = createButton(registry, 150.0F, 450.0F, 230.0F, 50.0F, "Join as Spectator",
                                            Color(100, 60, 100), [this]() { onToggleSpectator(); });

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
    if (state_ != State::ShowingRooms) {
        return;
    }

    if (event.type == EventType::MouseWheelScrolled) {
        constexpr float scrollSpeed  = 30.0F;
        constexpr float roomSpacing  = 70.0F;
        constexpr float clipTop      = 400.0F;
        constexpr float clipBottom   = 700.0F;
        constexpr float buttonHeight = 50.0F;
        float visibleAreaHeight      = clipBottom - clipTop - buttonHeight;
        float contentHeight = static_cast<float>(roomButtonEntities_.size()) * roomSpacing - roomSpacing + buttonHeight;
        float maxScroll     = std::max(0.0F, contentHeight - visibleAreaHeight);

        scrollOffset_ -= event.mouseWheelScroll.delta * scrollSpeed;
        scrollOffset_ = std::clamp(scrollOffset_, 0.0F, maxScroll);
        applyScrollOffset(registry);
    }

    if (event.type == EventType::KeyPressed) {
        constexpr float scrollSpeed  = 30.0F;
        constexpr float roomSpacing  = 70.0F;
        constexpr float clipTop      = 400.0F;
        constexpr float clipBottom   = 700.0F;
        constexpr float buttonHeight = 50.0F;
        float visibleAreaHeight      = clipBottom - clipTop - buttonHeight;
        float contentHeight = static_cast<float>(roomButtonEntities_.size()) * roomSpacing - roomSpacing + buttonHeight;
        float maxScroll     = std::max(0.0F, contentHeight - visibleAreaHeight);

        if (event.key.code == KeyCode::Up) {
            scrollOffset_ = std::max(0.0F, scrollOffset_ - scrollSpeed);
            applyScrollOffset(registry);
        } else if (event.key.code == KeyCode::Down) {
            scrollOffset_ = std::min(maxScroll, scrollOffset_ + scrollSpeed);
            applyScrollOffset(registry);
        }
    }
}

void LobbyMenu::render(Registry& registry, Window& window)
{
    if (state_ == State::ShowingCreateMenu) {
        if (!createRoomMenu_) {
            createRoomMenu_ = std::make_unique<CreateRoomMenu>(fonts_, textures_);
        }
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
                if (registry.isAlive(spectatorCheckboxEntity_))
                    registry.destroyEntity(spectatorCheckboxEntity_);

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

                std::string lobbyTitle = targetRoomType_ == RoomType::Ranked ? "Ranked Lobby" : "Game Lobby";
                titleEntity_           = createText(registry, 400.0F, 200.0F, lobbyTitle, 36, Color::White);
                statusEntity_ =
                    createText(registry, 400.0F, 250.0F, "Connecting to lobby...", 20, Color(200, 200, 200));

                if (targetRoomType_ == RoomType::Quickplay) {
                    createButtonEntity_ = createButton(registry, 400.0F, 320.0F, 200.0F, 50.0F, "Create Room",
                                                       Color(0, 120, 200), [this]() { onCreateRoomClicked(); });
                }

                refreshButtonEntity_ = createButton(registry, 620.0F, 320.0F, 180.0F, 50.0F, "Refresh",
                                                    Color(80, 80, 80), [this]() { onRefreshClicked(); });

                backButtonEntity_ = createButton(registry, 820.0F, 320.0F, 150.0F, 50.0F, "Back", Color(120, 50, 50),
                                                 [this]() { onBackClicked(); });

                if (targetRoomType_ == RoomType::Quickplay) {
                    filterFullButtonEntity_ = createButton(registry, 150.0F, 320.0F, 200.0F, 50.0F, "Hide Full",
                                                           Color(60, 100, 60), [this]() { onToggleFilterFull(); });

                    filterProtectedButtonEntity_ =
                        createButton(registry, 150.0F, 385.0F, 200.0F, 50.0F, "Hide Protected", Color(60, 100, 60),
                                     [this]() { onToggleFilterProtected(); });
                }
                spectatorCheckboxEntity_ = createButton(registry, 150.0F, 450.0F, 230.0F, 50.0F, "Join as Spectator",
                                                        Color(100, 60, 100), [this]() { onToggleSpectator(); });

                if (result.created) {
                    Logger::instance().info("[LobbyMenu] Creating room with configuration...");

                    auto* conn = getConnection();
                    if (!conn) {
                        state_                = State::Done;
                        result_.exitRequested = true;
                        return;
                    }

                    conn->sendCreateRoom(result.roomName, result.password, result.visibility);
                    state_           = State::Creating;
                    isCreating_      = true;
                    currentRoomName_ = result.roomName;
                    if (registry.has<TextComponent>(statusEntity_)) {
                        registry.get<TextComponent>(statusEntity_).content = "Creating room...";
                    }

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
        if (!passwordInputMenu_) {
            passwordInputMenu_ = std::make_unique<PasswordInputMenu>(fonts_, textures_);
        }
        if (passwordInputMenu_) {
            if (!passwordMenuInitialized_) {
                for (EntityId id : roomButtonEntities_) {
                    if (registry.isAlive(id))
                        registry.destroyEntity(id);
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
                if (registry.isAlive(spectatorCheckboxEntity_))
                    registry.destroyEntity(spectatorCheckboxEntity_);

                passwordInputMenu_->create(registry);
                passwordMenuInitialized_ = true;
            }

            passwordInputMenu_->render(registry, window);

            if (passwordInputMenu_->isDone()) {
                auto result = passwordInputMenu_->getResult(registry);
                passwordInputMenu_->destroy(registry);
                passwordMenuInitialized_ = false;

                backgroundEntity_      = createBackground(registry, textures_);
                logoEntity_            = createLogo(registry, textures_);
                std::string lobbyTitle = targetRoomType_ == RoomType::Ranked ? "Ranked Lobby" : "Game Lobby";
                titleEntity_           = createText(registry, 400.0F, 200.0F, lobbyTitle, 36, Color::White);
                statusEntity_ =
                    createText(registry, 400.0F, 250.0F, "Connecting to lobby...", 20, Color(200, 200, 200));
                if (targetRoomType_ == RoomType::Quickplay) {
                    createButtonEntity_ = createButton(registry, 400.0F, 320.0F, 200.0F, 50.0F, "Create Room",
                                                       Color(0, 120, 200), [this]() { onCreateRoomClicked(); });
                }
                refreshButtonEntity_ = createButton(registry, 620.0F, 320.0F, 180.0F, 50.0F, "Refresh",
                                                    Color(80, 80, 80), [this]() { onRefreshClicked(); });
                backButtonEntity_    = createButton(registry, 820.0F, 320.0F, 150.0F, 50.0F, "Back", Color(120, 50, 50),
                                                    [this]() { onBackClicked(); });
                if (targetRoomType_ == RoomType::Quickplay) {
                    filterFullButtonEntity_ = createButton(registry, 150.0F, 320.0F, 200.0F, 50.0F, "Hide Full",
                                                           Color(60, 100, 60), [this]() { onToggleFilterFull(); });
                    filterProtectedButtonEntity_ =
                        createButton(registry, 150.0F, 385.0F, 200.0F, 50.0F, "Hide Protected", Color(60, 100, 60),
                                     [this]() { onToggleFilterProtected(); });
                }
                spectatorCheckboxEntity_ = createButton(registry, 150.0F, 450.0F, 230.0F, 50.0F, "Join as Spectator",
                                                        Color(100, 60, 100), [this]() { onToggleSpectator(); });

                if (result.submitted) {
                    Logger::instance().info("[LobbyMenu] Password submitted, joining room...");

                    auto* conn = getConnection();
                    if (!conn || pendingJoinRoomIndex_ >= rooms_.size()) {
                        state_                = State::Done;
                        result_.exitRequested = true;
                        return;
                    }

                    extern bool g_joinAsSpectator;
                    g_joinAsSpectator = joinAsSpectator_;
                    Logger::instance().info("[LobbyMenu] Set g_joinAsSpectator to " +
                                            std::string(g_joinAsSpectator ? "true" : "false"));

                    const auto& room = rooms_[pendingJoinRoomIndex_];
                    conn->sendJoinRoom(room.roomId, result.password);
                    state_      = State::Joining;
                    isJoining_  = true;
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
    if (registry.isAlive(spectatorCheckboxEntity_) && registry.has<BoxComponent>(spectatorCheckboxEntity_)) {
        auto& box = registry.get<BoxComponent>(spectatorCheckboxEntity_);
        if (joinAsSpectator_) {
            box.fillColor    = Color(150, 100, 200);
            box.outlineColor = Color(200, 150, 255);
        } else {
            box.fillColor    = Color(100, 60, 100);
            box.outlineColor = Color(140, 100, 140);
        }
    }

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
            refreshTimeout_ -= dt;
            if (conn->hasRoomListResult()) {
                auto result     = conn->popRoomListResult();
                isRefreshing_   = false;
                refreshTimeout_ = 0.0F;
                if (result.has_value()) {
                    Logger::instance().info("[LobbyMenu] Room list received (" + std::to_string(result->rooms.size()) +
                                            " rooms)");
                    rooms_ = result->rooms;
                    if (state_ == State::Loading)
                        state_ = State::ShowingRooms;

                    if (targetRoomType_ == RoomType::Ranked) {
                        if (!rooms_.empty() && !isJoining_) {
                            Logger::instance().info("[LobbyMenu] Auto-joining ranked room " +
                                                    std::to_string(rooms_.front().roomId));
                            onJoinRoomClicked(0);
                            if (registry.has<TextComponent>(statusEntity_)) {
                                registry.get<TextComponent>(statusEntity_).content = "Joining ranked match...";
                            }
                        } else {
                            updateRoomListDisplay(registry);
                        }
                    } else {
                        updateRoomListDisplay(registry);
                    }
                } else {
                    Logger::instance().warn("[LobbyMenu] Room list result was empty");
                }
            } else if (refreshTimeout_ <= 0.0F) {
                Logger::instance().warn("[LobbyMenu] Room list refresh timed out, resetting isRefreshing");
                isRefreshing_   = false;
                refreshTimeout_ = 0.0F;
                if (state_ == State::Loading) {
                    if (registry.has<TextComponent>(statusEntity_)) {
                        registry.get<TextComponent>(statusEntity_).content = "Refresh timed out. Please try again.";
                    }
                }
            }
        }

        if (isGettingStats_) {
            if (conn->hasStatsResult()) {
                auto stats      = conn->popStatsResult();
                isGettingStats_ = false;
                if (stats.has_value()) {
                    Logger::instance().info("[LobbyMenu] Received stats - userId: " + std::to_string(stats->userId));

                    float winRate = 0.0F;
                    if (stats->gamesPlayed > 0) {
                        winRate = (static_cast<float>(stats->wins) / static_cast<float>(stats->gamesPlayed)) * 100.0F;
                    }

                    std::string username = std::string(stats->username);

                    if (registry.has<TextComponent>(statsUsernameEntity_)) {
                        registry.get<TextComponent>(statsUsernameEntity_).content = username;
                    }
                    if (registry.has<TextComponent>(statsGamesEntity_)) {
                        registry.get<TextComponent>(statsGamesEntity_).content =
                            "Games: " + std::to_string(stats->gamesPlayed);
                    }
                    if (registry.has<TextComponent>(statsWinsEntity_)) {
                        registry.get<TextComponent>(statsWinsEntity_).content = "Wins: " + std::to_string(stats->wins);
                    }
                    if (registry.has<TextComponent>(statsLossesEntity_)) {
                        registry.get<TextComponent>(statsLossesEntity_).content =
                            "Losses: " + std::to_string(stats->losses);
                    }
                    if (registry.has<TextComponent>(statsWinRateEntity_)) {
                        std::ostringstream oss;
                        oss << "Win Rate: " << std::fixed << std::setprecision(1) << winRate << "%";
                        registry.get<TextComponent>(statsWinRateEntity_).content = oss.str();
                    }
                    if (registry.has<TextComponent>(statsScoreEntity_)) {
                        registry.get<TextComponent>(statsScoreEntity_).content =
                            "Score: " + std::to_string(stats->totalScore);
                    }
                } else {
                    Logger::instance().warn("[LobbyMenu] Failed to load user stats");
                    if (registry.has<TextComponent>(statsUsernameEntity_)) {
                        registry.get<TextComponent>(statsUsernameEntity_).content = "Stats unavailable";
                    }
                }
            }
        }

        if (isCreating_) {
            if (conn->hasRoomCreatedResult()) {
                auto result = conn->popRoomCreatedResult();
                isCreating_ = false;
                if (result.has_value()) {
                    Logger::instance().info("[LobbyMenu] Room created: ID=" + std::to_string(result->roomId) +
                                            ", port=" + std::to_string(result->port));
                    result_.roomId   = result->roomId;
                    result_.gamePort = result->port;
                    isRoomHost_      = true;
                    state_           = State::InRoom;
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
                isJoining_  = false;
                if (result.has_value()) {
                    Logger::instance().info("[LobbyMenu] Joined room: ID=" + std::to_string(result->roomId));
                    result_.roomId   = result->roomId;
                    result_.gamePort = result->port;
                    state_           = State::InRoom;
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
        if (createRoomMenu_) {
            createRoomMenu_->destroy(registry);
            createRoomMenu_.reset();
            createMenuInitialized_ = false;
        }

        if (!roomWaitingMenu_) {
            roomWaitingMenu_ =
                std::make_unique<RoomWaitingMenu>(fonts_, textures_, result_.roomId, currentRoomName_, result_.gamePort,
                                                  isRoomHost_, targetRoomType_ == RoomType::Ranked, getConnection());
        }

        if (!roomWaitingMenuInitialized_) {
            for (EntityId id : roomButtonEntities_) {
                if (registry.isAlive(id))
                    registry.destroyEntity(id);
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
            if (registry.isAlive(spectatorCheckboxEntity_))
                registry.destroyEntity(spectatorCheckboxEntity_);

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
                result_.success             = true;
                result_.isHost              = isRoomHost_;
                result_.spectator           = joinAsSpectator_;
                result_.expectedPlayerCount = result.expectedPlayerCount;
                state_                      = State::Done;
            } else if (result.serverLost) {
                state_                = State::Done;
                result_.serverLost    = true;
                result_.backRequested = true;
            } else if (result.leaveRoom) {
                std::string lobbyTitle = targetRoomType_ == RoomType::Ranked ? "Ranked Lobby" : "Game Lobby";
                titleEntity_           = createText(registry, 400.0F, 200.0F, lobbyTitle, 36, Color::White);
                statusEntity_ =
                    createText(registry, 400.0F, 250.0F, "Connecting to lobby...", 20, Color(200, 200, 200));
                if (targetRoomType_ == RoomType::Quickplay) {
                    createButtonEntity_ = createButton(registry, 400.0F, 320.0F, 200.0F, 50.0F, "Create Room",
                                                       Color(0, 120, 200), [this]() { onCreateRoomClicked(); });
                }
                refreshButtonEntity_ = createButton(registry, 620.0F, 320.0F, 180.0F, 50.0F, "Refresh",
                                                    Color(80, 80, 80), [this]() { onRefreshClicked(); });
                backButtonEntity_    = createButton(registry, 820.0F, 320.0F, 150.0F, 50.0F, "Back", Color(120, 50, 50),
                                                    [this]() { onBackClicked(); });
                if (targetRoomType_ == RoomType::Quickplay) {
                    filterFullButtonEntity_ = createButton(registry, 150.0F, 320.0F, 200.0F, 50.0F, "Hide Full",
                                                           Color(60, 100, 60), [this]() { onToggleFilterFull(); });
                    filterProtectedButtonEntity_ =
                        createButton(registry, 150.0F, 385.0F, 200.0F, 50.0F, "Hide Protected", Color(60, 100, 60),
                                     [this]() { onToggleFilterProtected(); });
                }
                spectatorCheckboxEntity_ = createButton(registry, 150.0F, 450.0F, 230.0F, 50.0F, "Join as Spectator",
                                                        Color(100, 60, 100), [this]() { onToggleSpectator(); });

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

    if (!isGettingStats_ && sharedConnection_ && !statsLoaded_) {
        Logger::instance().info("[LobbyMenu] Loading stats with authenticated connection");
        loadAndDisplayStats(registry);
        statsLoaded_ = true;
    }

    refreshTimer_ += dt;
    if (refreshTimer_ >= kRefreshInterval) {
        refreshTimer_ = 0.0F;
        if (state_ == State::ShowingRooms || state_ == State::Loading) {
            if (!isRefreshing_)
                refreshRoomList();
        }
    }
}

void LobbyMenu::refreshRoomList()
{
    auto* conn = getConnection();
    if (!conn)
        return;

    if (isRefreshing_)
        return;

    conn->sendRequestRoomList();
    isRefreshing_   = true;
    refreshTimeout_ = 5.0F;
}

void LobbyMenu::onCreateRoomClicked()
{
    Logger::instance().info("[LobbyMenu] Opening create room menu...");
    state_ = State::ShowingCreateMenu;
}

void LobbyMenu::onJoinRoomClicked(std::size_t roomIndex)
{
    if (roomIndex >= rooms_.size())
        return;

    const auto& room = rooms_[roomIndex];

    if (room.passwordProtected) {
        Logger::instance().info("[LobbyMenu] Room " + std::to_string(room.roomId) + " is password-protected...");
        pendingJoinRoomIndex_ = roomIndex;
        state_                = State::ShowingPasswordInput;
        currentRoomName_      = room.roomName;
        return;
    }

    if (isJoining_)
        return;

    Logger::instance().info("[LobbyMenu] Joining room " + std::to_string(room.roomId) + " as " +
                            (joinAsSpectator_ ? "SPECTATOR" : "PLAYER") + "...");
    auto* conn = getConnection();
    if (!conn) {
        state_                = State::Done;
        result_.exitRequested = true;
        return;
    }

    extern bool g_joinAsSpectator;
    g_joinAsSpectator = joinAsSpectator_;
    Logger::instance().info("[LobbyMenu] Set g_joinAsSpectator to " +
                            std::string(g_joinAsSpectator ? "true" : "false"));

    conn->sendJoinRoom(room.roomId);
    isJoining_       = true;
    isRoomHost_      = false;
    currentRoomName_ = room.roomName;
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

void LobbyMenu::onToggleSpectator()
{
    joinAsSpectator_ = !joinAsSpectator_;
    Logger::instance().info("[LobbyMenu] Join as spectator: " + std::string(joinAsSpectator_ ? "YES" : "NO"));
}

bool LobbyMenu::shouldShowRoom(const RoomInfo& room) const
{
    if (room.roomType != targetRoomType_) {
        return false;
    }
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
    if (state_ == State::Creating) {
        if (registry.has<TextComponent>(statusEntity_)) {
            registry.get<TextComponent>(statusEntity_).content = "Creating room...";
        }
        return;
    }

    if (targetRoomType_ == RoomType::Ranked) {
        for (EntityId id : roomButtonEntities_) {
            if (registry.isAlive(id)) {
                registry.destroyEntity(id);
            }
        }
        roomButtonEntities_.clear();
        if (registry.has<TextComponent>(statusEntity_)) {
            registry.get<TextComponent>(statusEntity_).content = "Finding ranked match...";
        }
        return;
    }

    for (EntityId id : roomButtonEntities_) {
        if (registry.isAlive(id)) {
            registry.destroyEntity(id);
        }
    }
    roomButtonEntities_.clear();
    originalRoomButtonPositions_.clear();

    std::vector<std::size_t> sortedIndices;
    for (std::size_t i = 0; i < rooms_.size(); ++i) {
        if (shouldShowRoom(rooms_[i])) {
            sortedIndices.push_back(i);
        }
    }

    std::sort(sortedIndices.begin(), sortedIndices.end(), [this](std::size_t a, std::size_t b) {
        const auto& roomA = rooms_[a];
        const auto& roomB = rooms_[b];
        if (roomA.state == RoomState::Waiting && roomB.state != RoomState::Waiting) {
            return true;
        }
        if (roomA.state != RoomState::Waiting && roomB.state == RoomState::Waiting) {
            return false;
        }
        return false;
    });

    std::size_t displayIndex = 0;
    for (std::size_t roomIndex : sortedIndices) {
        createRoomButton(registry, rooms_[roomIndex], displayIndex, roomIndex);
        displayIndex++;
    }
    std::size_t visibleCount = sortedIndices.size();

    constexpr float roomSpacing  = 70.0F;
    constexpr float clipTop      = 400.0F;
    constexpr float clipBottom   = 700.0F;
    constexpr float buttonHeight = 50.0F;
    float visibleAreaHeight      = clipBottom - clipTop - buttonHeight;
    float contentHeight          = static_cast<float>(visibleCount) * roomSpacing - roomSpacing + buttonHeight;
    float maxScroll              = std::max(0.0F, contentHeight - visibleAreaHeight);
    scrollOffset_                = std::clamp(scrollOffset_, 0.0F, maxScroll);
    applyScrollOffset(registry);

    if (registry.has<TextComponent>(statusEntity_)) {
        std::size_t totalRoomsOfType = std::count_if(
            rooms_.begin(), rooms_.end(), [this](const RoomInfo& r) { return r.roomType == targetRoomType_; });
        if (totalRoomsOfType == 0) {
            registry.get<TextComponent>(statusEntity_).content = "No rooms available. Create one!";
        } else if (visibleCount == 0) {
            registry.get<TextComponent>(statusEntity_).content =
                "No rooms match filters. (" + std::to_string(totalRoomsOfType) + " total)";
        } else {
            registry.get<TextComponent>(statusEntity_).content =
                "Showing " + std::to_string(visibleCount) + " / " + std::to_string(totalRoomsOfType) + " room(s)";
        }
    }
}

void LobbyMenu::createRoomButton(Registry& registry, const RoomInfo& room, std::size_t displayIndex,
                                 std::size_t roomIndex)
{
    float startY  = 400.0F;
    float spacing = 70.0F;
    float y       = startY + static_cast<float>(displayIndex) * spacing;

    std::ostringstream label;
    if (room.roomType == RoomType::Ranked) {
        label << "[RANKED] ";
    }
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
                                     [this, roomIndex]() { onJoinRoomClicked(roomIndex); });

    roomButtonEntities_.push_back(buttonEntity);
    originalRoomButtonPositions_[buttonEntity] = y;
}

void LobbyMenu::loadAndDisplayStats(Registry& registry)
{
    (void) registry;
    auto* conn = getConnection();
    if (!conn) {
        Logger::instance().warn("[LobbyMenu] Cannot load stats: no lobby connection");
        return;
    }

    Logger::instance().info("[LobbyMenu] Requesting user stats...");
    conn->sendRequestStats();
    isGettingStats_ = true;
}

void LobbyMenu::applyScrollOffset(Registry& registry)
{
    constexpr float clipTop      = 400.0F;
    constexpr float clipBottom   = 700.0F;
    constexpr float buttonHeight = 50.0F;

    for (const auto& [entityId, originalY] : originalRoomButtonPositions_) {
        if (registry.isAlive(entityId) && registry.has<TransformComponent>(entityId)) {
            float newY                                   = originalY - scrollOffset_;
            registry.get<TransformComponent>(entityId).y = newY;

            if (registry.has<BoxComponent>(entityId)) {
                bool isVisible                               = (newY >= clipTop) && (newY + buttonHeight <= clipBottom);
                registry.get<BoxComponent>(entityId).visible = isVisible;
            }
        }
    }
}
