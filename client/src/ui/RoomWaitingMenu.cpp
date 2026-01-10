#include "ui/RoomWaitingMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/LobbyPackets.hpp"
#include "ui/NotificationData.hpp"

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
} // namespace

RoomWaitingMenu::RoomWaitingMenu(FontManager& fonts, TextureManager& textures, std::uint32_t roomId,
                                 std::uint16_t gamePort, bool isHost, LobbyConnection* lobbyConnection)
    : fonts_(fonts), textures_(textures), lobbyConnection_(lobbyConnection), roomId_(roomId), gamePort_(gamePort),
      isHost_(isHost)
{
    result_.roomId   = roomId;
    result_.gamePort = gamePort;
}

void RoomWaitingMenu::create(Registry& registry)
{
    if (!fonts_.has("ui")) {
        fonts_.load("ui", "client/assets/fonts/ui.ttf");
    }

    backgroundEntity_ = createBackground(registry, textures_);
    logoEntity_       = createLogo(registry, textures_);

    std::string roomTitle = "Room #" + std::to_string(roomId_);
    titleEntity_          = createText(registry, 450.0F, 200.0F, roomTitle, 36, Color::White);

    playerCountEntity_ = createText(registry, 400.0F, 260.0F, "Players: 1/4", 24, Color(200, 200, 200));

    if (isHost_) {
        startButtonEntity_ = createButton(registry, 400.0F, 600.0F, 200.0F, 50.0F, "Start Game", Color(0, 150, 80),
                                          [this]() { onStartGameClicked(); });
    }

    leaveButtonEntity_ = createButton(registry, 620.0F, 600.0F, 150.0F, 50.0F, "Leave Room", Color(120, 50, 50),
                                      [this]() { onLeaveRoomClicked(); });

    updatePlayerList(registry);
}

void RoomWaitingMenu::destroy(Registry& registry)
{
    if (registry.isAlive(backgroundEntity_))
        registry.destroyEntity(backgroundEntity_);
    if (registry.isAlive(logoEntity_))
        registry.destroyEntity(logoEntity_);
    if (registry.isAlive(titleEntity_))
        registry.destroyEntity(titleEntity_);
    if (registry.isAlive(playerCountEntity_))
        registry.destroyEntity(playerCountEntity_);
    if (registry.isAlive(startButtonEntity_))
        registry.destroyEntity(startButtonEntity_);
    if (registry.isAlive(leaveButtonEntity_))
        registry.destroyEntity(leaveButtonEntity_);

    for (auto entityId : playerTextEntities_) {
        if (registry.isAlive(entityId))
            registry.destroyEntity(entityId);
    }
    playerTextEntities_.clear();

    for (auto entityId : playerBadgeEntities_) {
        if (registry.isAlive(entityId))
            registry.destroyEntity(entityId);
    }
    playerBadgeEntities_.clear();

    for (auto entityId : kickButtonEntities_) {
        if (registry.isAlive(entityId))
            registry.destroyEntity(entityId);
    }
    kickButtonEntities_.clear();
}

bool RoomWaitingMenu::isDone() const
{
    return done_;
}

void RoomWaitingMenu::handleEvent(Registry& registry, const Event& event)
{
    (void) registry;
    (void) event;
}

void RoomWaitingMenu::render(Registry& registry, Window& window)
{
    (void) window;

    if (lobbyConnection_) {
        ThreadSafeQueue<NotificationData> dummyQueue;
        lobbyConnection_->poll(dummyQueue);
    }

    if (lobbyConnection_ && lobbyConnection_->isGameStarting()) {
        result_.expectedPlayerCount = lobbyConnection_->getExpectedPlayerCount();
        Logger::instance().info("[RoomWaitingMenu] Game starting detected with " +
                                std::to_string(result_.expectedPlayerCount) + " players, exiting lobby...");
        result_.startGame = true;
        done_             = true;
        return;
    }

    if (lobbyConnection_ && lobbyConnection_->wasKicked()) {
        Logger::instance().warn("[RoomWaitingMenu] Player was kicked from the room!");
        result_.leaveRoom = true;
        done_             = true;
        return;
    }

    if (registry.has<TextComponent>(playerCountEntity_)) {
        registry.get<TextComponent>(playerCountEntity_).content = "Players: " + std::to_string(players_.size()) + "/4";
    }
}

void RoomWaitingMenu::update(Registry& registry, float dt)
{
    if (lobbyConnection_) {
        if (isRefreshingPlayers_) {
            if (lobbyConnection_->hasPlayerListResult()) {
                auto playerListOpt = lobbyConnection_->popPlayerListResult();
                isRefreshingPlayers_ = false;

                if (playerListOpt.has_value()) {
                     consecutiveFailures_ = 0;
                     Logger::instance().info("[RoomWaitingMenu] Received player list: " + std::to_string(playerListOpt->size()) + " players");
                     players_.clear();
                     for (const auto& playerInfo : *playerListOpt) {
                        PlayerInfo info;
                        info.playerId = playerInfo.playerId;
                        info.name = "Player " + std::to_string(playerInfo.playerId);
                        info.isHost = playerInfo.isHost;
                        players_.push_back(info);
                     }
                     updatePlayerList(registry);
                } else {
                     Logger::instance().warn("[RoomWaitingMenu] Failed to get player list");
                     consecutiveFailures_++;
                }
            }
        }
    }

    updateTimer_ += dt;
    if (updateTimer_ >= kUpdateInterval) {
        updateTimer_ = 0.0F;
        if (lobbyConnection_ && !isRefreshingPlayers_) {
             lobbyConnection_->sendRequestPlayerList(roomId_);
             isRefreshingPlayers_ = true;
        }

        if (consecutiveFailures_ >= 2) {
             Logger::instance().error("[RoomWaitingMenu] Connection to lobby server lost (2 timeouts)");
             result_.serverLost = true;
             result_.leaveRoom  = true;
             done_              = true;
        }
    }

    if (registry.has<TextComponent>(playerCountEntity_)) {
        registry.get<TextComponent>(playerCountEntity_).content = "Players: " + std::to_string(players_.size()) + "/4";
    }
}

void RoomWaitingMenu::updatePlayerList(Registry& registry)
{
    for (auto entityId : playerTextEntities_) {
        if (registry.isAlive(entityId))
            registry.destroyEntity(entityId);
    }
    for (auto entityId : playerBadgeEntities_) {
        if (registry.isAlive(entityId))
            registry.destroyEntity(entityId);
    }
    for (auto entityId : kickButtonEntities_) {
        if (registry.isAlive(entityId))
            registry.destroyEntity(entityId);
    }
    playerTextEntities_.clear();
    playerBadgeEntities_.clear();
    kickButtonEntities_.clear();

    float startY = 320.0F;
    for (size_t i = 0; i < players_.size(); ++i) {
        const auto& player = players_[i];

        Color playerColor = player.isHost ? Color(255, 215, 0) : Color(200, 200, 200);
        auto nameEntity   = createText(registry, 350.0F, startY + (i * 50.0F), player.name, 22, playerColor);
        playerTextEntities_.push_back(nameEntity);

        if (player.isHost) {
            auto badgeEntity = createText(registry, 550.0F, startY + (i * 50.0F), "[OWNER]", 20, Color(255, 215, 0));
            playerBadgeEntities_.push_back(badgeEntity);
        }

        if (isHost_ && !player.isHost) {
            auto kickButton =
                createButton(registry, 680.0F, startY + (i * 50.0F), 80.0F, 35.0F, "Kick", Color(180, 50, 50),
                             [this, playerId = player.playerId]() { onKickPlayerClicked(playerId); });
            kickButtonEntities_.push_back(kickButton);
        }
    }
}

void RoomWaitingMenu::onStartGameClicked()
{
    Logger::instance().info("[RoomWaitingMenu] Start game clicked (Host only)");

    if (lobbyConnection_) {
        lobbyConnection_->sendNotifyGameStarting(roomId_);
        Logger::instance().info("[RoomWaitingMenu] Waiting for server confirmation...");
    }
}

void RoomWaitingMenu::onLeaveRoomClicked()
{
    Logger::instance().info("[RoomWaitingMenu] Leave room clicked");

    if (lobbyConnection_) {
        lobbyConnection_->sendLeaveRoom();
    }

    result_.leaveRoom = true;
    done_             = true;
}

void RoomWaitingMenu::onKickPlayerClicked(std::uint32_t playerId)
{
    Logger::instance().info("[RoomWaitingMenu] Kick player " + std::to_string(playerId) + " clicked");

    if (lobbyConnection_) {
        lobbyConnection_->sendKickPlayer(roomId_, playerId);
    }
}
