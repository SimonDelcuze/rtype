#include "ui/RoomWaitingMenuRanked.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/InputFieldComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "ui/NotificationData.hpp"
#include "graphics/GraphicsFactory.hpp"
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
        auto& t         = registry.emplace<TransformComponent>(entity);
        t.x             = x;
        t.y             = y;
        auto text       = TextComponent::create("ui", size, color);
        text.content    = content;
        registry.emplace<TextComponent>(entity, text);
        return entity;
    }

    EntityId createPanel(Registry& registry, float x, float y, float w, float h, const Color& fill)
    {
        EntityId e = registry.createEntity();
        auto& t    = registry.emplace<TransformComponent>(e);
        t.x        = x;
        t.y        = y;
        auto box   = BoxComponent::create(w, h, fill, Color(fill.r + 20, fill.g + 20, fill.b + 20, fill.a));
        registry.emplace<BoxComponent>(e, box);
        return e;
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
} // namespace

RoomWaitingMenuRanked::RoomWaitingMenuRanked(FontManager& fonts, TextureManager& textures, std::uint32_t roomId,
                                             const std::string& roomName, std::uint16_t gamePort,
                                             LobbyConnection* lobbyConnection)
    : fonts_(fonts), textures_(textures), lobbyConnection_(lobbyConnection)
{
    result_.roomId   = roomId;
    result_.gamePort = gamePort;
    roomId_          = roomId;
    roomName_        = roomName;
    gamePort_        = gamePort;
}

void RoomWaitingMenuRanked::create(Registry& registry)
{
    background_ = createBackground(registry, textures_);
    logo_       = createLogo(registry, textures_);

    std::string title = roomName_ + " (#" + std::to_string(roomId_) + ")";
    title_            = createText(registry, 470.0F, 200.0F, title, 32, Color::White);
    status_           = createText(registry, 470.0F, 240.0F, "Waiting for players...", 18, Color(200, 200, 200));
    playerCount_      = createText(registry, 470.0F, 270.0F, "Players: 0/4", 18, Color(200, 200, 200));
    timerLabel_       = createText(registry, 420.0F, 50.0F, "", 24, Color(255, 100, 100));

    readyButton_ = registry.createEntity();
    auto& tr     = registry.emplace<TransformComponent>(readyButton_);
    tr.x         = 442.0F;
    tr.y         = 650.0F;
    auto readyBox = BoxComponent::create(320.0F, 50.0F, Color(200, 50, 50), Color(200, 50, 50));
    registry.emplace<BoxComponent>(readyButton_, readyBox);
    registry.emplace<ButtonComponent>(readyButton_, ButtonComponent::create("", [this, &registry]() {
                                          isReady_ = !isReady_;
                                          if (lobbyConnection_) {
                                              lobbyConnection_->sendSetReady(roomId_, isReady_);
                                          }
                                          if (registry.isAlive(readyButton_) && registry.isAlive(readyButtonText_)) {
                                              auto& text = registry.get<TextComponent>(readyButtonText_);
                                              text.content = isReady_ ? "NOT READY" : "READY";
                                              auto& box = registry.get<BoxComponent>(readyButton_);
                                              Color c = isReady_ ? Color(50, 200, 50) : Color(200, 50, 50);
                                              box.fillColor = c;
                                              box.outlineColor = c;
                                              auto& textTr = registry.get<TransformComponent>(readyButtonText_);
                                              textTr.x = isReady_ ? 532.0F : 557.0F;
                                          }
                                      }));

    readyButtonText_ = createText(registry, 557.0F, 663.0F, "READY", 24, Color::White);
    createPanel(registry, 40.0F, 180.0F, 360.0F, 180.0F, Color(50, 70, 100, 200));
    createText(registry, 60.0F, 192.0F, "Score Leaderboard", 20, Color(200, 230, 255));
    createText(registry, 60.0F, 225.0F, "No scores yet", 16, Color(210, 220, 230));

    createPanel(registry, 40.0F, 380.0F, 360.0F, 240.0F, Color(40, 60, 90, 200));
    createText(registry, 60.0F, 392.0F, "Rank Leaderboard", 20, Color(200, 230, 255));
    createText(registry, 60.0F, 425.0F, "No ranks yet", 16, Color(210, 220, 230));
    createPanel(registry, 430.0F, 320.0F, 340.0F, 320.0F, Color(25, 35, 55, 200));
    createText(registry, 450.0F, 332.0F, "Players", 20, Color(180, 220, 255));
    createPanel(registry, 800.0F, 250.0F, 460.0F, 400.0F, Color(30, 30, 30, 180));
    createText(registry, 820.0F, 260.0F, "Chat", 28, Color(150, 200, 255));

    buildChatUI(registry);
    refreshPlayers(registry);
}

void RoomWaitingMenuRanked::destroy(Registry& registry)
{
    auto destroyIf = [&](EntityId id) {
        if (registry.isAlive(id))
            registry.destroyEntity(id);
    };
    destroyIf(background_);
    destroyIf(logo_);
    destroyIf(title_);
    destroyIf(playerCount_);
    destroyIf(status_);
    destroyIf(readyButton_);
    destroyIf(readyButtonText_);
    destroyIf(timerLabel_);
    destroyIf(chatBg_);
    destroyIf(chatInput_);
    destroyIf(chatSend_);
    for (auto id : playerEntities_) {
        destroyIf(id);
    }
    for (auto id : chatEntities_) {
        destroyIf(id);
    }
    for (auto id : chatMessageEntities_) {
        destroyIf(id);
    }
    playerEntities_.clear();
    chatEntities_.clear();
    chatMessageEntities_.clear();
}

bool RoomWaitingMenuRanked::isDone() const
{
    return result_.startGame || result_.leaveRoom || result_.serverLost;
}

void RoomWaitingMenuRanked::handleEvent(Registry& registry, const Event& event)
{
    if (event.type == EventType::KeyPressed && event.key.code == KeyCode::Enter) {
        onSendChatClicked(registry);
    }
}

void RoomWaitingMenuRanked::render(Registry&, Window&)
{}

void RoomWaitingMenuRanked::update(Registry& registry, float dt)
{
    if (lobbyConnection_) {
        ThreadSafeQueue<NotificationData> dummy;
        lobbyConnection_->poll(dummy);
        if (lobbyConnection_->isGameStarting()) {
            result_.expectedPlayerCount = lobbyConnection_->getExpectedPlayerCount();
            result_.startGame = true;
            return;
        }
        if (lobbyConnection_->wasKicked()) {
            result_.leaveRoom = true;
            return;
        }

        if (isRefreshing_) {
            if (lobbyConnection_->hasPlayerListResult()) {
                auto list = lobbyConnection_->popPlayerListResult();
                isRefreshing_ = false;
                if (list.has_value()) {
                    consecutiveFailures_ = 0;
                    players_.clear();
                    for (const auto& p : *list) {
                        PlayerRow row;
                        row.playerId = p.playerId;
                        row.name = std::string(p.name);
                        row.rankName = "Prey";
                        row.elo = 1000;
                        row.isReady = p.isReady;
                        players_.push_back(row);
                    }
                    buildPlayerList(registry);
                } else {
                    consecutiveFailures_++;
                }
            }
        }

        std::uint8_t countdown = lobbyConnection_->getRoomCountdown();
        if (countdown > 0) {
            if (registry.has<TextComponent>(timerLabel_)) {
                auto& text = registry.get<TextComponent>(timerLabel_);
                text.content = "Game starting in: " + std::to_string(countdown) + "s";
            }
        } else {
            if (registry.has<TextComponent>(timerLabel_)) {
                registry.get<TextComponent>(timerLabel_).content = "";
            }
        }
    }

    updateTimer_ += dt;
    if (updateTimer_ >= kUpdateInterval) {
        updateTimer_ = 0.0F;
        refreshPlayers(registry);

        if (consecutiveFailures_ >= 2) {
            result_.serverLost = true;
            return;
        }
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
        for (auto id : chatMessageEntities_) {
            if (registry.isAlive(id))
                registry.destroyEntity(id);
        }
        chatMessageEntities_.clear();

        for (std::size_t i = 0; i < chatHistory_.size(); ++i) {
            auto msgEntity = createText(registry, 820.0F, 300.0F + (static_cast<float>(i) * 25.0F), chatHistory_[i], 18,
                                        Color(220, 220, 220));
            chatMessageEntities_.push_back(msgEntity);
        }
    }
}

void RoomWaitingMenuRanked::buildPlayerList(Registry& registry)
{
    for (auto id : playerEntities_) {
        if (registry.isAlive(id))
            registry.destroyEntity(id);
    }
    playerEntities_.clear();

    float startY = 370.0F;
    for (std::size_t i = 0; i < players_.size(); ++i) {
        const auto& p = players_[i];
        std::string readyText = p.isReady ? " [READY]" : "";
        Color textColor = p.isReady ? Color(100, 255, 100) : Color(220, 220, 220);
        auto name     = createText(registry, 450.0F, startY + static_cast<float>(i) * 40.0F,
                                   p.name + " (" + p.rankName + " " + std::to_string(p.elo) + ")" + readyText, 18,
                                   textColor);
        playerEntities_.push_back(name);
    }

    if (registry.has<TextComponent>(playerCount_)) {
        registry.get<TextComponent>(playerCount_).content = "Players: " + std::to_string(players_.size()) + "/4";
    }
}

void RoomWaitingMenuRanked::buildChatUI(Registry& registry)
{
    chatBg_ = registry.createEntity();
    auto& t = registry.emplace<TransformComponent>(chatBg_);
    t.x     = 800.0F;
    t.y     = 250.0F;
    auto box =
        BoxComponent::create(460.0F, 400.0F, Color(30, 30, 30, 180), Color(60, 60, 60, 180));
    registry.emplace<BoxComponent>(chatBg_, box);

    auto chatField             = InputFieldComponent::create("", 120);
    chatField.placeholder      = "Type message...";
    chatField.centerVertically = true;
    chatInput_                 = registry.createEntity();
    auto& ti                   = registry.emplace<TransformComponent>(chatInput_);
    ti.x                       = 820.0F;
    ti.y                       = 600.0F;
    auto chatBox               = BoxComponent::create(300.0F, 40.0F, Color(30, 30, 30), Color(60, 60, 60));
    registry.emplace<BoxComponent>(chatInput_, chatBox);
    registry.emplace<InputFieldComponent>(chatInput_, chatField);

    chatSend_ = registry.createEntity();
    auto& ts  = registry.emplace<TransformComponent>(chatSend_);
    ts.x      = 1160.0F;
    ts.y      = 600.0F;
    auto sendBox =
        BoxComponent::create(80.0F, 40.0F, Color(0, 150, 80), Color(0, 180, 100));
    registry.emplace<BoxComponent>(chatSend_, sendBox);
    registry.emplace<ButtonComponent>(chatSend_, ButtonComponent::create("Send", [this, &registry]() {
                                          onSendChatClicked(registry);
                                      }));
}

void RoomWaitingMenuRanked::onSendChatClicked(Registry& registry)
{
    if (chatInput_ != 0 && registry.has<InputFieldComponent>(chatInput_)) {
        auto& input = registry.get<InputFieldComponent>(chatInput_);
        if (!input.value.empty() && lobbyConnection_) {
            lobbyConnection_->sendChatMessage(roomId_, input.value);
            input.value.clear();
        }
    }
}

void RoomWaitingMenuRanked::refreshPlayers(Registry& registry)
{
    (void)registry;
    if (!lobbyConnection_ || isRefreshing_)
        return;

    lobbyConnection_->sendRequestPlayerList(roomId_);
    isRefreshing_ = true;
}
