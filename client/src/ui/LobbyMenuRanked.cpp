#include "ui/LobbyMenuRanked.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "network/LeaderboardPacket.hpp"
#include "ui/RoomWaitingMenu.hpp"

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

    EntityId createPanel(Registry& registry, float x, float y, float w, float h, const Color& fill)
    {
        EntityId entity = registry.createEntity();
        auto& t         = registry.emplace<TransformComponent>(entity);
        t.x             = x;
        t.y             = y;
        auto box        = BoxComponent::create(w, h, fill, Color(fill.r + 20, fill.g + 20, fill.b + 20, fill.a));
        registry.emplace<BoxComponent>(entity, box);
        return entity;
    }

    EntityId createText(Registry& registry, float x, float y, const std::string& content, unsigned int size,
                        const Color& color)
    {
        EntityId entity = registry.createEntity();
        auto& t         = registry.emplace<TransformComponent>(entity);
        t.x             = x;
        t.y             = y;
        auto text       = TextComponent::create("ui", size, color);
        text.content    = content;
        registry.emplace<TextComponent>(entity, text);
        return entity;
    }

    EntityId createButton(Registry& registry, float x, float y, float w, float h, const std::string& label,
                          const Color& fill, std::function<void()> onClick)
    {
        EntityId entity = registry.createEntity();
        auto& t         = registry.emplace<TransformComponent>(entity);
        t.x             = x;
        t.y             = y;
        auto box =
            BoxComponent::create(w, h, fill,
                                 Color(static_cast<std::uint8_t>(fill.r + 40), static_cast<std::uint8_t>(fill.g + 40),
                                       static_cast<std::uint8_t>(fill.b + 40)));
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        return entity;
    }
} // namespace

LobbyMenuRanked::LobbyMenuRanked(FontManager& fonts, TextureManager& textures, const IpEndpoint& lobbyEndpoint,
                                 ThreadSafeQueue<NotificationData>& broadcastQueue,
                                 const std::atomic<bool>& runningFlag, LobbyConnection* sharedConnection)
    : fonts_(fonts), textures_(textures), lobbyEndpoint_(lobbyEndpoint), broadcastQueue_(broadcastQueue),
      runningFlag_(runningFlag), sharedConnection_(sharedConnection), ownsConnection_(sharedConnection == nullptr)
{}

void LobbyMenuRanked::create(Registry& registry)
{
    registry_ = &registry;
    if (!fonts_.has("ui")) {
        fonts_.load("ui", "client/assets/fonts/ui.ttf");
    }

    buildLayout(registry);

    if (!textures_.has("rank_prey"))
        textures_.load("rank_prey", "client/assets/ranks/prey.png");
    if (!textures_.has("rank_hunter"))
        textures_.load("rank_hunter", "client/assets/ranks/hunter.png");
    if (!textures_.has("rank_predator"))
        textures_.load("rank_predator", "client/assets/ranks/predator.png");
    if (!textures_.has("rank_apex"))
        textures_.load("rank_apex", "client/assets/ranks/apex.png");

    if (sharedConnection_) {
        ownsConnection_ = false;
    } else {
        lobbyConnection_ = std::make_unique<LobbyConnection>(lobbyEndpoint_, runningFlag_);
        if (!lobbyConnection_->connect()) {
            Logger::instance().error("[LobbyMenuRanked] Failed to connect to lobby server");
            result_.exitRequested = true;
            state_                = State::Done;
            return;
        }
        ownsConnection_ = true;
    }

    auto* conn = sharedConnection_ ? sharedConnection_ : lobbyConnection_.get();
    if (conn) {
        conn->sendRequestLeaderboard();
        leaderboardTimer_ = 0.0F;
    }

    state_ = State::Idle;
    updateStatus(registry, "Ready for ranked");
}

void LobbyMenuRanked::buildLayout(Registry& registry)
{
    layoutBuilt_ = true;
    background_  = createBackground(registry, textures_);
    logo_        = createLogo(registry, textures_);
    title_       = createText(registry, 400.0F, 200.0F, "Ranked Lobby", 36, Color::White);
    status_      = createText(registry, 400.0F, 240.0F, "Connecting...", 18, Color(200, 200, 200));

    leftBoard_  = createPanel(registry, 120.0F, 300.0F, 330.0F, 240.0F, Color(20, 30, 50, 180));
    rightBoard_ = createPanel(registry, 820.0F, 300.0F, 330.0F, 240.0F, Color(30, 40, 60, 180));

    leftTitle_  = createText(registry, 150.0F, 320.0F, "Rank Leaderboard", 20, Color(180, 220, 255));
    rightTitle_ = createText(registry, 850.0F, 320.0F, "Score Leaderboard", 20, Color(180, 220, 255));

    leaderboardEntities_.push_back(createText(registry, 140.0F, 360.0F, "No ranks yet", 16, Color(210, 220, 230)));
    leaderboardEntities_.push_back(createText(registry, 840.0F, 360.0F, "No scores yet", 16, Color(210, 220, 230)));

    findBtn_ = createButton(registry, 500.0F, 360.0F, 200.0F, 60.0F, "Find Game", Color(0, 120, 200),
                            [this]() { onFindGameClicked(); });
    backBtn_ = createButton(registry, 520.0F, 44	0.0F, 160.0F, 50.0F, "Back", Color(120, 50, 50),
                            [this]() { onBackClicked(); });
}

void LobbyMenuRanked::destroy(Registry& registry)
{
    registry_ = nullptr;
    destroyLobbyEntities(registry);
    waitingMenu_.reset();
    if (ownsConnection_ && lobbyConnection_) {
        lobbyConnection_->disconnect();
        lobbyConnection_.reset();
    }
}

bool LobbyMenuRanked::isDone() const
{
    return state_ == State::Done;
}

void LobbyMenuRanked::handleEvent(Registry&, const Event&) {}

void LobbyMenuRanked::render(Registry& registry, Window& window)
{
    if (state_ == State::InRoom && waitingMenu_) {
        waitingMenu_->render(registry, window);
    }
}

void LobbyMenuRanked::update(Registry& registry, float dt)
{
    dotTimer_ += dt;
    if ((state_ == State::Finding || state_ == State::Joining) && registry_ != nullptr) {
        if (dotTimer_ >= 0.3F) {
            dotTimer_ = 0.0F;
            dotCount_ = (dotCount_ % 3) + 1;
            std::string dots(static_cast<std::size_t>(dotCount_), '.');
            updateStatus(*registry_, "Joining a room" + dots);
        }
    }

    auto* conn = sharedConnection_ ? sharedConnection_ : lobbyConnection_.get();
    if (conn) {
        conn->poll(broadcastQueue_);
        if (conn->isServerLost()) {
            result_.serverLost = true;
            state_             = State::Done;
            return;
        }

        if (state_ != State::InRoom) {
            leaderboardTimer_ += dt;
            if (leaderboardTimer_ >= 5.0F) {
                leaderboardTimer_ = 0.0F;
                conn->sendRequestLeaderboard();
            }

            if (conn->hasLeaderboardResult()) {
                auto res = conn->popLeaderboardResult();
                if (res.has_value()) {
                    updateLeaderboardUI(registry, *res);
                }
            }
        }
    }

    if (state_ == State::InRoom && waitingMenu_) {
        waitingMenu_->update(registry, dt);
        if (waitingMenu_->isDone()) {
            auto res = waitingMenu_->getResult(registry);
            waitingMenu_->destroy(registry);
            waitingMenu_.reset();
            if (res.startGame) {
                result_.success             = true;
                result_.expectedPlayerCount = res.expectedPlayerCount;
                result_.gamePort            = res.gamePort;
                result_.roomId              = res.roomId;
                state_                      = State::Done;
            } else if (res.serverLost) {
                result_.serverLost = true;
                state_             = State::Done;
            } else if (res.leaveRoom) {
                state_ = State::Idle;
                if (!layoutBuilt_) {
                    buildLayout(registry);
                }
                updateStatus(registry, "Ready for ranked");
            }
        }
        return;
    }

    if (state_ == State::Finding || state_ == State::Joining) {
        requestTimer_ += dt;
        if (conn && conn->hasRoomListResult()) {
            auto roomRes = conn->popRoomListResult();
            if (roomRes.has_value()) {
                rooms_ = roomRes->rooms;
                RoomInfo chosen{};
                bool found = false;
                for (auto& r : rooms_) {
                    if (r.roomType == RoomType::Ranked) {
                        chosen = r;
                        found  = true;
                        break;
                    }
                }
                if (found) {
                    Logger::instance().info("[LobbyMenuRanked] Joining ranked room " + std::to_string(chosen.roomId));
                    conn->sendJoinRoom(chosen.roomId);
                    state_ = State::Joining;
                } else {
                    updateStatus(registry, "No ranked room available, retrying...");
                    state_        = State::Idle;
                    requestTimer_ = 0.0F;
                    if (!layoutBuilt_) {
                        buildLayout(registry);
                    }
                }
            } else {
                updateStatus(registry, "Failed to fetch rooms");
                state_        = State::Idle;
                requestTimer_ = 0.0F;
                if (!layoutBuilt_) {
                    buildLayout(registry);
                }
            }
        } else if (conn && state_ == State::Finding && requestTimer_ > 1.0F) {
            requestTimer_ = 0.0F;
            refreshRooms();
        }

        if (conn && conn->hasJoinRoomResult()) {
            auto res = conn->popJoinRoomResult();
            if (res.has_value()) {
                result_.roomId   = res->roomId;
                result_.gamePort = res->port;

                for (auto id : leaderboardEntities_) {
                    if (registry.isAlive(id))
                        registry.destroyEntity(id);
                }
                leaderboardEntities_.clear();

                transitionToWaiting(registry);
                return;
            } else {
                updateStatus(registry, "Join failed");
                state_        = State::Idle;
                requestTimer_ = 0.0F;
            }
        }
    }
}

void LobbyMenuRanked::refreshRooms()
{
    auto* conn = sharedConnection_ ? sharedConnection_ : lobbyConnection_.get();
    if (!conn)
        return;
    conn->sendRequestRoomList();
}

void LobbyMenuRanked::onFindGameClicked()
{
    Logger::instance().info("[LobbyMenuRanked] Find Game clicked");
    state_ = State::Finding;
    if (registry_) {
        updateStatus(*registry_, "Joining a room...");
    }
    refreshRooms();
}

void LobbyMenuRanked::onBackClicked()
{
    Logger::instance().info("[LobbyMenuRanked] Back clicked");
    result_.backRequested = true;
    state_                = State::Done;
}

void LobbyMenuRanked::updateStatus(Registry& registry, const std::string& text)
{
    if (registry.has<TextComponent>(status_)) {
        registry.get<TextComponent>(status_).content = text;
    }
}

void LobbyMenuRanked::transitionToWaiting(Registry& registry)
{
    destroyLobbyEntities(registry);
    waitingMenu_ =
        std::make_unique<RoomWaitingMenuRanked>(fonts_, textures_, result_.roomId, "Ranked", result_.gamePort,
                                                sharedConnection_ ? sharedConnection_ : lobbyConnection_.get());
    waitingMenu_->create(registry);
    waitingMenuInit_ = true;
    state_           = State::InRoom;
}

void LobbyMenuRanked::updateLeaderboardUI(Registry& registry, const LeaderboardResponseData& data)
{
    auto getRankTexture = [](int elo) -> std::string {
        if (elo >= 1900)
            return "rank_apex";
        if (elo >= 1500)
            return "rank_predator";
        if (elo >= 1200)
            return "rank_hunter";
        return "rank_prey";
    };

    for (auto id : leaderboardEntities_) {
        if (registry.isAlive(id))
            registry.destroyEntity(id);
    }
    leaderboardEntities_.clear();

    float spacing = 35.0F;

    float scoreX = 840.0F;
    float scoreY = 360.0F;
    for (std::size_t i = 0; i < data.topScore.size(); ++i) {
        std::string name(data.topScore[i].username);
        if (name.empty())
            continue;
        std::string line = std::to_string(i + 1) + ". " + name + ": " + std::to_string(data.topScore[i].value);
        leaderboardEntities_.push_back(
            createText(registry, scoreX, scoreY + (i * spacing), line, 16, Color(210, 220, 230)));
    }

    float rankX = 140.0F;
    float rankY = 360.0F;
    for (std::size_t i = 0; i < data.topElo.size(); ++i) {
        std::string name(data.topElo[i].username);
        if (name.empty())
            continue;

        float rowY = rankY + (i * spacing);

        std::string rankNum = std::to_string(i + 1) + ". ";
        leaderboardEntities_.push_back(createText(registry, rankX, rowY, rankNum, 16, Color(210, 220, 230)));

        std::string rankTex = getRankTexture(data.topElo[i].value);
        if (textures_.has(rankTex)) {
            auto tex      = textures_.get(rankTex);
            EntityId icon = registry.createEntity();
            auto& t       = registry.emplace<TransformComponent>(icon);
            t.x           = rankX + 25.0F;
            t.y           = rowY - 5.0F;
            t.scaleX      = 0.08F;
            t.scaleY      = 0.08F;
            registry.emplace<SpriteComponent>(icon, SpriteComponent(tex));
            registry.emplace<LayerComponent>(icon, LayerComponent::create(RenderLayer::UI));
            leaderboardEntities_.push_back(icon);
        }

        std::string info = name + " (" + std::to_string(data.topElo[i].value) + ")";
        leaderboardEntities_.push_back(createText(registry, rankX + 60.0F, rowY, info, 16, Color(210, 220, 230)));
    }
}

void LobbyMenuRanked::destroyLobbyEntities(Registry& registry)
{
    for (EntityId id :
         {background_, logo_, title_, status_, findBtn_, backBtn_, leftBoard_, rightBoard_, leftTitle_, rightTitle_}) {
        if (registry.isAlive(id))
            registry.destroyEntity(id);
    }
    for (auto id : leaderboardEntities_) {
        if (registry.isAlive(id))
            registry.destroyEntity(id);
    }
    leaderboardEntities_.clear();
    background_ = logo_ = title_ = status_ = findBtn_ = backBtn_ = leftBoard_ = rightBoard_ = leftTitle_ = rightTitle_ =
        0;
    layoutBuilt_ = false;
}
