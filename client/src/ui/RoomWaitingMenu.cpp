#include "ui/RoomWaitingMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/FocusableComponent.hpp"
#include "components/InputFieldComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/TextureManager.hpp"
#include "network/LobbyPackets.hpp"
#include "ui/NotificationData.hpp"

#include <algorithm>
#include <cmath>
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

    EntityId createArrowButton(Registry& registry, float x, float y, float width, float height,
                               const std::string& label, std::function<void()> onClick)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto box       = BoxComponent::create(width, height, Color(70, 70, 70), Color(110, 110, 110));
        box.focusColor = Color(100, 200, 255, 160);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        return entity;
    }

    EntityId createIconButton(Registry& registry, float x, float y, float size, TextureManager& textures,
                              const std::string& textureId, const Color& fill, std::function<void()> onClick,
                              EntityId& iconEntityOut, float spriteScale, float offsetX, float offsetY)
    {
        EntityId entity = createButton(registry, x, y, size, size, "", fill, std::move(onClick));
        iconEntityOut   = 0;
        auto tex        = textures.get(textureId);
        if (tex == nullptr) {
            Logger::instance().error("[RoomWaitingMenu] Missing texture: " + textureId);
            return entity;
        }

        if (registry.has<BoxComponent>(entity)) {
            auto& box          = registry.get<BoxComponent>(entity);
            box.fillColor.a    = 0;
            box.outlineColor.a = 0;
        }

        iconEntityOut = registry.createEntity();
        auto& t       = registry.emplace<TransformComponent>(iconEntityOut);
        float texW    = static_cast<float>(tex->getSize().x);
        float texH    = static_cast<float>(tex->getSize().y);
        float targetW = texW * spriteScale;
        float targetH = texH * spriteScale;
        t.x           = x + (size - targetW) * 0.5F + offsetX;
        t.y           = y + (size - targetH) * 0.5F + offsetY;
        t.scaleX      = spriteScale;
        t.scaleY      = spriteScale;
        Logger::instance().info("[RoomWaitingMenu] Icon " + textureId + " pos=(" + std::to_string(t.x) + "," +
                                std::to_string(t.y) + ") scale=" + std::to_string(spriteScale));

        SpriteComponent sprite(tex);
        sprite.setScale(1.0F, 1.0F);
        registry.emplace<SpriteComponent>(iconEntityOut, sprite);
        registry.emplace<LayerComponent>(iconEntityOut, LayerComponent::create(100));
        return entity;
    }

    EntityId createInputField(Registry& registry, float x, float y, float width, float height,
                              InputFieldComponent field, int tabOrder)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto box       = BoxComponent::create(width, height, Color(50, 50, 50), Color(100, 100, 100));
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<InputFieldComponent>(entity, field);
        registry.emplace<FocusableComponent>(entity, FocusableComponent::create(tabOrder));
        return entity;
    }
    void wrapText(const std::string& text, float maxWidth, FontManager& fonts, std::vector<std::string>& lines)
    {
        auto font = fonts.get("ui");
        if (!font) {
            lines.push_back(text);
            return;
        }

        GraphicsFactory factory;
        auto textObj = factory.createText();
        textObj->setFont(*font);
        textObj->setCharacterSize(18);

        std::string currentLine;
        std::string word;
        std::stringstream ss(text);

        while (ss >> word) {
            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
            textObj->setString(testLine);
            if (textObj->getGlobalBounds().width > maxWidth) {
                if (!currentLine.empty()) {
                    lines.push_back(currentLine);
                    currentLine = word;
                } else {
                    std::string fragmented;
                    for (char c : word) {
                        textObj->setString(fragmented + c);
                        if (textObj->getGlobalBounds().width > maxWidth) {
                            lines.push_back(fragmented);
                            fragmented = c;
                        } else {
                            fragmented += c;
                        }
                    }
                    currentLine = fragmented;
                }
            } else {
                currentLine = testLine;
            }
        }
        if (!currentLine.empty())
            lines.push_back(currentLine);
    }

    std::string toStringPct(float value)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0F;
        return ss.str();
    }

    std::string toStringLives(std::uint8_t lives)
    {
        return std::to_string(lives);
    }

    struct DifficultyPreset
    {
        float enemyMultiplier{1.0F};
        float playerSpeedMultiplier{1.0F};
        float scoreMultiplier{1.0F};
        std::uint8_t lives{3};
    };

    DifficultyPreset presetFromMode(RoomDifficulty difficulty)
    {
        DifficultyPreset p{};
        switch (difficulty) {
            case RoomDifficulty::Noob:
                p.enemyMultiplier       = 0.5F;
                p.playerSpeedMultiplier = 1.0F;
                p.scoreMultiplier       = 0.5F;
                p.lives                 = 3;
                break;
            case RoomDifficulty::Hell:
                p.enemyMultiplier       = 1.0F;
                p.playerSpeedMultiplier = 1.0F;
                p.scoreMultiplier       = 1.0F;
                p.lives                 = 2;
                break;
            case RoomDifficulty::Nightmare:
                p.enemyMultiplier       = 1.5F;
                p.playerSpeedMultiplier = 0.67F;
                p.scoreMultiplier       = 1.5F;
                p.lives                 = 1;
                break;
            case RoomDifficulty::Custom:
            default:
                break;
        }
        return p;
    }

    std::string difficultyName(RoomDifficulty difficulty)
    {
        switch (difficulty) {
            case RoomDifficulty::Noob:
                return "Noob";
            case RoomDifficulty::Hell:
                return "Hell";
            case RoomDifficulty::Nightmare:
                return "Nightmare";
            case RoomDifficulty::Custom:
                return "Custom";
        }
        return "Unknown";
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
    difficulty_            = RoomDifficulty::Hell;
    enemyMultiplier_       = 1.0F;
    playerSpeedMultiplier_ = 1.0F;
    scoreMultiplier_       = 1.0F;
    playerLives_           = 2;

    if (!fonts_.has("ui")) {
        fonts_.load("ui", "client/assets/fonts/ui.ttf");
    }

    buildChrome(registry);
    buildControlButtons(registry);
    buildDifficultyUI(registry);
    buildChatUI(registry);

    updatePlayerList(registry);
}

void RoomWaitingMenu::buildDifficultyUI(Registry& registry)
{
    float baseX            = 30.0F;
    difficultyTitleEntity_ = createText(registry, baseX, 220.0F, "Game Config", 22, Color(220, 220, 220));

    const std::array<std::pair<std::string, std::string>, 4> textureInfos{
        std::make_pair("diff_noob", "client/assets/other/noob.png"),
        std::make_pair("diff_hell", "client/assets/other/hell.png"),
        std::make_pair("diff_nightmare", "client/assets/other/nightmare.png"),
        std::make_pair("diff_custom", "client/assets/other/custom.png")};
    for (const auto& info : textureInfos) {
        textures_.load(info.first, info.second);
        Logger::instance().info("[RoomWaitingMenu] Loaded icon " + info.first + " from " + info.second);
    }

    std::array<float, 4> iconScales{0.4F, 0.4F, 0.4F, 0.4F};
    std::array<float, 4> iconOffsetX{0.0F, 0.0F, 0.0F, 0.0F};
    std::array<float, 4> iconOffsetY{0.0F, 0.0F, 0.0F, 0.0F};

    for (std::size_t i = 0; i < textureInfos.size(); ++i) {
        difficultyButtons_[i] = createIconButton(
            registry, baseX + static_cast<float>(i) * 90.0F, 255.0F, 64.0F, textures_, textureInfos[i].first,
            Color(50, 70, 90),
            [this, i]() {
                if (!isHost_)
                    return;
                switch (i) {
                    case 0:
                        setDifficulty(RoomDifficulty::Noob);
                        break;
                    case 1:
                        setDifficulty(RoomDifficulty::Hell);
                        break;
                    case 2:
                        setDifficulty(RoomDifficulty::Nightmare);
                        break;
                    case 3:
                        setDifficulty(RoomDifficulty::Custom);
                        break;
                }
            },
            difficultyIcons_[i], iconScales[i], iconOffsetX[i], iconOffsetY[i]);
    }

    selectedDifficultyLabel_ = createText(registry, baseX, 333.0F, "Selected: Noob", 18, Color(190, 220, 255));

    configTitleEntity_ = createText(registry, baseX, 375.0F, "Stats", 18, Color(200, 200, 200));

    auto makeField = [&](const std::string& label, const std::string& value, float y) {
        ConfigRow row;
        row.label              = createText(registry, baseX, y, label, 16, Color(200, 200, 200));
        auto field             = InputFieldComponent::create(value, 8);
        field.centerVertically = true;
        row.input              = createInputField(registry, 200.0F + baseX, y - 6.0F, 100.0F, 36.0F, field, 0);
        return row;
    };

    enemyRow_  = makeField("Enemy stats", toStringPct(enemyMultiplier_), 425.0F);
    playerRow_ = makeField("Player speed", toStringPct(playerSpeedMultiplier_), 475.0F);
    scoreRow_  = makeField("Score gain", toStringPct(scoreMultiplier_), 525.0F);
    livesRow_  = makeField("Lives", toStringLives(playerLives_), 575.0F);

    auto disableCaret = [&](const ConfigRow& row) {
        if (registry.has<InputFieldComponent>(row.input)) {
            registry.get<InputFieldComponent>(row.input).editable = false;
        }
    };
    disableCaret(enemyRow_);
    disableCaret(playerRow_);
    disableCaret(scoreRow_);
    disableCaret(livesRow_);

    auto addArrows = [&](ConfigRow& row, float y, float baseXOffset) {
        float btnW  = 26.0F;
        float btnH  = 18.0F;
        row.upBtn   = createArrowButton(registry, baseX + baseXOffset - 1.0F, y - 6.0F, btnW, btnH, "/\\", []() {});
        row.downBtn = createArrowButton(registry, baseX + baseXOffset - 1.0F, y + 14.0F, btnW, btnH, "\\/", []() {});
        if (registry.has<ButtonComponent>(row.upBtn))
            registry.get<ButtonComponent>(row.upBtn).textOffsetX = -1.5F;
        if (registry.has<ButtonComponent>(row.downBtn))
            registry.get<ButtonComponent>(row.downBtn).textOffsetX = -1.5F;
        if (registry.has<ButtonComponent>(row.upBtn)) {
            auto& btn          = registry.get<ButtonComponent>(row.upBtn);
            btn.autoRepeat     = true;
            btn.repeatDelay    = 0.25F;
            btn.repeatInterval = 0.07F;
        }
        if (registry.has<ButtonComponent>(row.downBtn)) {
            auto& btn          = registry.get<ButtonComponent>(row.downBtn);
            btn.autoRepeat     = true;
            btn.repeatDelay    = 0.25F;
            btn.repeatInterval = 0.07F;
        }
    };

    addArrows(enemyRow_, 425.0F, 310.0F);
    addArrows(playerRow_, 475.0F, 310.0F);
    addArrows(livesRow_, 575.0F, 310.0F);

    auto attachArrow = [&](EntityId btn, std::function<void()> fn) {
        if (registry.has<ButtonComponent>(btn)) {
            registry.get<ButtonComponent>(btn).onClick = [this, &registry, fn]() {
                if (!isHost_ || difficulty_ != RoomDifficulty::Custom)
                    return;
                fn();
                updateDifficultyUI(registry);
            };
        }
    };

    attachArrow(enemyRow_.upBtn, [this]() { enemyMultiplier_ = std::min(enemyMultiplier_ + 0.05F, 2.0F); });
    attachArrow(enemyRow_.downBtn, [this]() { enemyMultiplier_ = std::max(enemyMultiplier_ - 0.05F, 0.5F); });
    attachArrow(playerRow_.upBtn,
                [this]() { playerSpeedMultiplier_ = std::min(playerSpeedMultiplier_ + 0.05F, 2.0F); });
    attachArrow(playerRow_.downBtn,
                [this]() { playerSpeedMultiplier_ = std::max(playerSpeedMultiplier_ - 0.05F, 0.5F); });
    attachArrow(livesRow_.upBtn,
                [this]() { playerLives_ = static_cast<std::uint8_t>(std::min<int>(playerLives_ + 1, 10)); });
    attachArrow(livesRow_.downBtn,
                [this]() { playerLives_ = static_cast<std::uint8_t>(std::max<int>(playerLives_ - 1, 1)); });

    setDifficulty(difficulty_);
}

void RoomWaitingMenu::destroyDifficultyUI(Registry& registry)
{
    if (registry.isAlive(difficultyTitleEntity_))
        registry.destroyEntity(difficultyTitleEntity_);
    if (registry.isAlive(configTitleEntity_))
        registry.destroyEntity(configTitleEntity_);
    if (registry.isAlive(selectedDifficultyLabel_))
        registry.destroyEntity(selectedDifficultyLabel_);
    for (auto id : difficultyButtons_) {
        if (registry.isAlive(id))
            registry.destroyEntity(id);
    }
    for (auto id : difficultyIcons_) {
        if (registry.isAlive(id))
            registry.destroyEntity(id);
    }
    auto destroyRow = [&](const ConfigRow& row) {
        if (registry.isAlive(row.label))
            registry.destroyEntity(row.label);
        if (registry.isAlive(row.input))
            registry.destroyEntity(row.input);
        if (registry.isAlive(row.upBtn))
            registry.destroyEntity(row.upBtn);
        if (registry.isAlive(row.downBtn))
            registry.destroyEntity(row.downBtn);
    };
    destroyRow(enemyRow_);
    destroyRow(playerRow_);
    destroyRow(scoreRow_);
    destroyRow(livesRow_);
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

    destroyChatUI(registry);

    destroyDifficultyUI(registry);
}

bool RoomWaitingMenu::isDone() const
{
    return done_;
}

void RoomWaitingMenu::handleEvent(Registry& registry, const Event& event)
{
    if (event.type == EventType::KeyPressed && event.key.code == KeyCode::Enter) {
        onSendChatClicked(registry);
    }
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

    updateDifficultyUI(registry);
}

void RoomWaitingMenu::update(Registry& registry, float dt)
{
    if (lobbyConnection_) {
        if (lobbyConnection_->hasRoomConfigUpdate()) {
            auto updOpt = lobbyConnection_->popRoomConfigUpdate();
            if (updOpt.has_value() && updOpt->roomId == roomId_) {
                suppressSend_          = true;
                difficulty_            = updOpt->mode;
                enemyMultiplier_       = updOpt->enemyMultiplier;
                playerSpeedMultiplier_ = updOpt->playerSpeedMultiplier;
                scoreMultiplier_       = updOpt->scoreMultiplier;
                playerLives_           = updOpt->playerLives;
                lastSentConfig_.mode   = difficulty_;
                lastSentConfig_.enemy  = enemyMultiplier_;
                lastSentConfig_.player = playerSpeedMultiplier_;
                lastSentConfig_.score  = scoreMultiplier_;
                lastSentConfig_.lives  = playerLives_;
                updateDifficultyUI(registry);
            }
        }
        if (isRefreshingPlayers_) {
            if (lobbyConnection_->hasPlayerListResult()) {
                auto playerListOpt   = lobbyConnection_->popPlayerListResult();
                isRefreshingPlayers_ = false;

                if (playerListOpt.has_value()) {
                    consecutiveFailures_ = 0;
                    Logger::instance().info("[RoomWaitingMenu] Received player list: " +
                                            std::to_string(playerListOpt->size()) + " players");
                    players_.clear();
                    for (const auto& playerInfo : *playerListOpt) {
                        PlayerInfo info;
                        info.playerId = playerInfo.playerId;
                        info.name     = std::string(playerInfo.name);
                        info.isHost   = playerInfo.isHost;
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
    if (lobbyConnection_ && lobbyConnection_->hasNewChatMessages()) {
        auto newMsgs = lobbyConnection_->popChatMessages();
        for (const auto& msg : newMsgs) {
            std::string formatted = "[" + std::string(msg.playerName) + "] " + std::string(msg.message);
            std::vector<std::string> lines;
            wrapText(formatted, 420.0F, fonts_, lines);
            for (const auto& line : lines) {
                chatHistory_.push_back(line);
                if (chatHistory_.size() > kMaxChatMessages) {
                    chatHistory_.erase(chatHistory_.begin());
                }
            }
        }
        for (auto entityId : chatMessageEntities_) {
            if (registry.isAlive(entityId))
                registry.destroyEntity(entityId);
        }
        chatMessageEntities_.clear();

        for (std::size_t i = 0; i < chatHistory_.size(); ++i) {
            auto msgEntity = createText(registry, 820.0F, 300.0F + (static_cast<float>(i) * 25.0F), chatHistory_[i], 18,
                                        Color(220, 220, 220));
            chatMessageEntities_.push_back(msgEntity);
        }
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

    float startY = 340.0F;
    float startX = 480.0F;
    for (size_t i = 0; i < players_.size(); ++i) {
        const auto& player = players_[i];

        Color playerColor = player.isHost ? Color(255, 215, 0) : Color(200, 200, 200);
        auto nameEntity   = createText(registry, startX, startY + (i * 50.0F), player.name, 22, playerColor);
        playerTextEntities_.push_back(nameEntity);

        if (player.isHost) {
            auto badgeEntity =
                createText(registry, startX + 150.0F, startY + (i * 50.0F), "[OWNER]", 20, Color(255, 215, 0));
            playerBadgeEntities_.push_back(badgeEntity);
        }

        if (isHost_ && !player.isHost) {
            auto kickButton =
                createButton(registry, startX + 150.0F, startY + (i * 50.0F), 80.0F, 35.0F, "Kick", Color(180, 50, 50),
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

void RoomWaitingMenu::onSendChatClicked(Registry& registry)
{
    if (chatInputField_ != 0 && registry.has<InputFieldComponent>(chatInputField_)) {
        auto& input = registry.get<InputFieldComponent>(chatInputField_);
        if (!input.value.empty()) {
            lobbyConnection_->sendChatMessage(roomId_, input.value);
            input.value.clear();
        }
    }
}

void RoomWaitingMenu::buildChrome(Registry& registry)
{
    backgroundEntity_ = createBackground(registry, textures_);
    logoEntity_       = createLogo(registry, textures_);

    std::string roomTitle = "Room #" + std::to_string(roomId_);
    titleEntity_          = createText(registry, 450.0F, 200.0F, roomTitle, 36, Color::White);

    playerCountEntity_ = createText(registry, 400.0F, 260.0F, "Players: 1/4", 24, Color(200, 200, 200));
}

void RoomWaitingMenu::buildControlButtons(Registry& registry)
{
    if (isHost_) {
        startButtonEntity_ = createButton(registry, 400.0F, 600.0F, 200.0F, 50.0F, "Start Game", Color(0, 150, 80),
                                          [this]() { onStartGameClicked(); });
    }

    leaveButtonEntity_ = createButton(registry, 620.0F, 600.0F, 150.0F, 50.0F, "Leave Room", Color(120, 50, 50),
                                      [this]() { onLeaveRoomClicked(); });
}

void RoomWaitingMenu::buildChatUI(Registry& registry)
{
    chatBackgroundEntity_ = registry.createEntity();
    auto& chatBgTransform = registry.emplace<TransformComponent>(chatBackgroundEntity_);
    chatBgTransform.x     = 800.0F;
    chatBgTransform.y     = 250.0F;
    auto chatBgBox        = BoxComponent::create(460.0F, 400.0F, Color(30, 30, 30, 180), Color(60, 60, 60, 180));
    registry.emplace<BoxComponent>(chatBackgroundEntity_, chatBgBox);

    createText(registry, 820.0F, 260.0F, "Chat", 28, Color(150, 200, 255));
    auto chatField             = InputFieldComponent::create("", 120);
    chatField.placeholder      = "Type message...";
    chatField.centerVertically = true;
    chatInputField_            = createInputField(registry, 820.0F, 600.0F, 300.0F, 40.0F, chatField, 0);

    sendButtonEntity_ = createButton(registry, 1160.0F, 600.0F, 80.0F, 40.0F, "Send", Color(0, 150, 80),
                                     [this, &registry]() { onSendChatClicked(registry); });
}

void RoomWaitingMenu::destroyChatUI(Registry& registry)
{
    if (registry.isAlive(chatInputField_))
        registry.destroyEntity(chatInputField_);
    if (registry.isAlive(chatBackgroundEntity_))
        registry.destroyEntity(chatBackgroundEntity_);
    if (registry.isAlive(sendButtonEntity_))
        registry.destroyEntity(sendButtonEntity_);
    for (auto entityId : chatMessageEntities_) {
        if (registry.isAlive(entityId))
            registry.destroyEntity(entityId);
    }
    chatMessageEntities_.clear();
}

void RoomWaitingMenu::setDifficulty(RoomDifficulty difficulty)
{
    difficulty_        = difficulty;
    result_.difficulty = difficulty_;

    if (difficulty_ != RoomDifficulty::Custom) {
        DifficultyPreset preset = presetFromMode(difficulty_);
        enemyMultiplier_        = preset.enemyMultiplier;
        playerSpeedMultiplier_  = preset.playerSpeedMultiplier;
        scoreMultiplier_        = preset.scoreMultiplier;
        playerLives_            = preset.lives;
    } else {
        scoreMultiplier_ = 1.0F;
    }
}

void RoomWaitingMenu::setInputValue(Registry& registry, EntityId inputId, const std::string& value)
{
    if (!registry.has<InputFieldComponent>(inputId))
        return;
    registry.get<InputFieldComponent>(inputId).value = value;
}

void RoomWaitingMenu::updateDifficultyUI(Registry& registry)
{
    for (std::size_t i = 0; i < difficultyButtons_.size(); ++i) {
        if (registry.has<BoxComponent>(difficultyButtons_[i])) {
            auto& box        = registry.get<BoxComponent>(difficultyButtons_[i]);
            bool active      = static_cast<int>(difficulty_) == static_cast<int>(i);
            box.fillColor    = active ? Color(0, 150, 80, 40) : Color(50, 70, 90, 20);
            box.outlineColor = active ? Color(0, 180, 110, 180) : Color(80, 90, 110, 120);
        }
        if (difficultyIcons_[i] != 0 && registry.has<TransformComponent>(difficultyIcons_[i])) {
            auto& t = registry.get<TransformComponent>(difficultyIcons_[i]);
            (void) t;
        }
    }

    const bool isCustom = difficulty_ == RoomDifficulty::Custom;
    const bool canEdit  = isHost_;

    if (isCustom && canEdit) {
        enemyMultiplier_       = std::clamp(enemyMultiplier_, 0.5F, 2.0F);
        playerSpeedMultiplier_ = std::clamp(playerSpeedMultiplier_, 0.5F, 2.0F);
        scoreMultiplier_       = 1.0F;
        playerLives_           = static_cast<std::uint8_t>(std::clamp<int>(playerLives_, 1, 6));
    } else if (!isCustom) {
        DifficultyPreset preset = presetFromMode(difficulty_);
        enemyMultiplier_        = preset.enemyMultiplier;
        playerSpeedMultiplier_  = preset.playerSpeedMultiplier;
        scoreMultiplier_        = preset.scoreMultiplier;
        playerLives_            = preset.lives;
    } else {
    }

    result_.enemyMultiplier       = enemyMultiplier_;
    result_.playerSpeedMultiplier = playerSpeedMultiplier_;
    result_.scoreMultiplier       = scoreMultiplier_;
    result_.playerLives           = playerLives_;

    maybeSendRoomConfig();

    if (registry.has<TextComponent>(selectedDifficultyLabel_)) {
        registry.get<TextComponent>(selectedDifficultyLabel_).content = "Selected: " + difficultyName(difficulty_);
    }

    setInputValue(registry, enemyRow_.input, toStringPct(enemyMultiplier_));
    setInputValue(registry, playerRow_.input, toStringPct(playerSpeedMultiplier_));
    setInputValue(registry, scoreRow_.input, toStringPct(scoreMultiplier_));
    setInputValue(registry, livesRow_.input, toStringLives(playerLives_));

    auto setRowState = [&](const ConfigRow& row, bool locked) {
        if (registry.has<BoxComponent>(row.input)) {
            auto& box        = registry.get<BoxComponent>(row.input);
            box.fillColor    = locked ? Color(60, 60, 60, 160) : Color(50, 50, 50);
            box.outlineColor = locked ? Color(90, 90, 90) : Color(100, 100, 100);
        }
        if (registry.has<InputFieldComponent>(row.input)) {
            auto& input = registry.get<InputFieldComponent>(row.input);
            if (locked) {
                input.focused = false;
            }
        }
        if (registry.has<TextComponent>(row.label)) {
            registry.get<TextComponent>(row.label).color = locked ? Color(140, 140, 140) : Color(200, 200, 200);
        }
    };

    auto setArrowsVisible = [&](const ConfigRow& row, bool visible) {
        auto applyTo = [&](EntityId id) {
            if (!registry.isAlive(id))
                return;
            if (registry.has<TransformComponent>(id)) {
                auto& t  = registry.get<TransformComponent>(id);
                t.scaleX = visible ? 1.0F : 0.0F;
                t.scaleY = visible ? 1.0F : 0.0F;
            }
            if (registry.has<BoxComponent>(id)) {
                auto& b          = registry.get<BoxComponent>(id);
                b.fillColor.a    = visible ? 255 : 0;
                b.outlineColor.a = visible ? 255 : 0;
                b.focusColor.a   = visible ? 200 : 0;
            }
            if (registry.has<TextComponent>(id)) {
                auto& txt   = registry.get<TextComponent>(id);
                txt.color.a = visible ? 255 : 0;
            }
        };
        applyTo(row.upBtn);
        applyTo(row.downBtn);

        auto clearTextIfHidden = [&](EntityId id, bool isUp) {
            if (!registry.isAlive(id))
                return;
            if (registry.has<TextComponent>(id)) {
                auto& txt = registry.get<TextComponent>(id);
                if (!visible) {
                    txt.content.clear();
                    txt.color.a = 0;
                } else {
                    txt.content = isUp ? "/\\" : "\\/";
                    txt.color.a = 255;
                }
            }
            if (registry.has<ButtonComponent>(id)) {
                auto& btn = registry.get<ButtonComponent>(id);
                btn.label = visible ? (isUp ? "/\\" : "\\/") : "";
            }
        };
        clearTextIfHidden(row.upBtn, true);
        clearTextIfHidden(row.downBtn, false);
    };

    bool lockedRows = !canEdit || !isCustom;
    setRowState(enemyRow_, lockedRows);
    setRowState(playerRow_, lockedRows);
    setRowState(scoreRow_, true);
    setRowState(livesRow_, lockedRows);

    bool showArrows = canEdit && isCustom;
    setArrowsVisible(enemyRow_, showArrows);
    setArrowsVisible(playerRow_, showArrows);
    setArrowsVisible(livesRow_, showArrows);
}

void RoomWaitingMenu::maybeSendRoomConfig()
{
    if (!isHost_ || lobbyConnection_ == nullptr)
        return;

    if (suppressSend_) {
        suppressSend_ = false;
        return;
    }

    bool changed =
        (difficulty_ != lastSentConfig_.mode) || (std::abs(enemyMultiplier_ - lastSentConfig_.enemy) > 0.001F) ||
        (std::abs(playerSpeedMultiplier_ - lastSentConfig_.player) > 0.001F) ||
        (std::abs(scoreMultiplier_ - lastSentConfig_.score) > 0.001F) || (playerLives_ != lastSentConfig_.lives);
    if (!changed)
        return;

    lastSentConfig_.mode   = difficulty_;
    lastSentConfig_.enemy  = enemyMultiplier_;
    lastSentConfig_.player = playerSpeedMultiplier_;
    lastSentConfig_.score  = scoreMultiplier_;
    lastSentConfig_.lives  = playerLives_;

    lobbyConnection_->sendRoomConfig(roomId_, difficulty_, enemyMultiplier_, playerSpeedMultiplier_, scoreMultiplier_,
                                     playerLives_);
}
