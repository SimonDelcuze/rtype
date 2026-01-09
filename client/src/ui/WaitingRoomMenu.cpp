#include "ui/WaitingRoomMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "network/ClientInit.hpp"

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
                        Color color)
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

    EntityId createButton(Registry& registry, float x, float y, const std::string& label, Color fill,
                          std::function<void()> onClick)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto box =
            BoxComponent::create(200.0F, 60.0F, fill,
                                 Color(static_cast<std::uint8_t>(fill.r + 40), static_cast<std::uint8_t>(fill.g + 40),
                                       static_cast<std::uint8_t>(fill.b + 40)));
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        return entity;
    }

} // namespace

WaitingRoomMenu::WaitingRoomMenu(FontManager& fonts, TextureManager& textures, UdpSocket& socket,
                                 const IpEndpoint& server, std::atomic<bool>& allReadyFlag,
                                 std::atomic<int>& countdownValueFlag, std::atomic<bool>& gameStartFlag)
    : fonts_(fonts), textures_(textures), socket_(socket), server_(server), allReadyFlag_(allReadyFlag),
      countdownValueFlag_(countdownValueFlag), gameStartFlag_(gameStartFlag)
{}

void WaitingRoomMenu::create(Registry& registry)
{
    if (!fonts_.has("ui"))
        fonts_.load("ui", "client/assets/fonts/ui.ttf");

    createBackground(registry, textures_);
    createLogo(registry, textures_);

    createText(registry, 450.0F, 280.0F, "Waiting Room", 48, Color(255, 255, 255));

    readyButton_   = createButton(registry, 540.0F, 400.0F, "READY", Color(0, 180, 80), [this]() { onReadyClicked(); });
    waitingText_   = createText(registry, 280.0F, 500.0F, "", 28, Color(200, 200, 200));
    countdownText_ = createText(registry, 600.0F, 350.0F, "", 96, Color(255, 220, 0));
}

void WaitingRoomMenu::destroy(Registry& registry)
{
    registry.clear();
}

bool WaitingRoomMenu::isDone() const
{
    return done_;
}

void WaitingRoomMenu::handleEvent(Registry&, const Event&) {}

void WaitingRoomMenu::render(Registry&, Window&) {}

void WaitingRoomMenu::update(Registry& registry, float dt)
{
    if (buttonClicked_ && readyButton_ != 0) {
        hideButton(registry);
    }

    if (gameStartFlag_.load()) {
        Logger::instance().info("GameStart received, exiting waiting room");
        state_ = State::Done;
        done_  = true;
        return;
    }

    switch (state_) {
        case State::WaitingForClick:
            break;
        case State::WaitingForPlayers:
            readyRetryTimer_ += dt;
            if (readyRetryTimer_ >= 0.5F) {
                sendClientReady(server_, socket_);
                readyRetryTimer_ = 0.0F;
            }
            updateDotAnimation(dt);
            updateWaitingText(registry);
            if (allReadyFlag_.load()) {
                Logger::instance().info("All players ready, waiting for countdown from server");
                state_ = State::Countdown;
            }
            break;
        case State::Countdown:
            updateCountdownFromServer(registry);
            break;
        case State::Done:
            break;
    }
}

WaitingRoomMenu::Result WaitingRoomMenu::getResult(Registry&) const
{
    Result result;
    result.ready = (state_ == State::Done);
    return result;
}

void WaitingRoomMenu::onReadyClicked()
{
    if (state_ != State::WaitingForClick)
        return;

    Logger::instance().info("Ready button clicked, sending CLIENT_READY");
    sendClientReady(server_, socket_);
    state_         = State::WaitingForPlayers;
    buttonClicked_ = true;
}

void WaitingRoomMenu::updateDotAnimation(float dt)
{
    dotTimer_ += dt;
    if (dotTimer_ >= 0.4F) {
        dotTimer_ = 0.0F;
        dotCount_ = (dotCount_ % 3) + 1;
    }
}

void WaitingRoomMenu::updateWaitingText(Registry& registry)
{
    if (waitingText_ == 0 || !registry.has<TextComponent>(waitingText_))
        return;

    std::string dots(static_cast<std::size_t>(dotCount_), '.');
    auto& text   = registry.get<TextComponent>(waitingText_);
    text.content = "Waiting for all players to be ready" + dots;
}

void WaitingRoomMenu::updateCountdownFromServer(Registry& registry)
{
    int serverVal = countdownValueFlag_.load();

    if (serverVal != lastCountdownVal_ && serverVal >= 0) {
        lastCountdownVal_ = serverVal;
        Logger::instance().info("Displaying countdown: " + std::to_string(serverVal));

        if (waitingText_ != 0 && registry.has<TextComponent>(waitingText_)) {
            auto& text   = registry.get<TextComponent>(waitingText_);
            text.content = "";
        }

        if (countdownText_ != 0 && registry.has<TextComponent>(countdownText_)) {
            auto& text   = registry.get<TextComponent>(countdownText_);
            text.content = std::to_string(serverVal);
        }
    }
}

void WaitingRoomMenu::hideButton(Registry& registry)
{
    if (readyButton_ != 0 && registry.isAlive(readyButton_)) {
        registry.destroyEntity(readyButton_);
        readyButton_ = 0;
    }
}
