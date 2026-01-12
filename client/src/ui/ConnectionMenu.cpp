#include "ui/ConnectionMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/FocusableComponent.hpp"
#include "components/InputFieldComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"

#include <cmath>
#include <utility>

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

    EntityId createLabel(Registry& registry, float x, float y, const std::string& content)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto text    = TextComponent::create("ui", 24, Color(200, 200, 200));
        text.content = content;
        registry.emplace<TextComponent>(entity, text);
        return entity;
    }

    EntityId createInputField(Registry& registry, float x, float y, InputFieldComponent field, int tabOrder)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto box       = BoxComponent::create(400.0F, 50.0F, Color(50, 50, 50), Color(100, 100, 100));
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<InputFieldComponent>(entity, field);
        registry.emplace<FocusableComponent>(entity, FocusableComponent::create(tabOrder));
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
            BoxComponent::create(180.0F, 50.0F, fill,
                                 Color(static_cast<std::uint8_t>(fill.r + 40), static_cast<std::uint8_t>(fill.g + 40),
                                       static_cast<std::uint8_t>(fill.b + 40)));
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        return entity;
    }

} // namespace

ConnectionMenu::ConnectionMenu(FontManager& fonts, TextureManager& textures, std::string initialError)
    : fonts_(fonts), textures_(textures), initialError_(std::move(initialError))
{}

void ConnectionMenu::create(Registry& registry)
{
    done_         = false;
    openSettings_ = false;
    connecting_   = false;

    if (!fonts_.has("ui"))
        fonts_.load("ui", "client/assets/fonts/ui.ttf");

    createBackground(registry, textures_);
    createLogo(registry, textures_);
    createLabel(registry, 440.0F, 250.0F, "Server IP:");
    createLabel(registry, 440.0F, 390.0F, "Port:");
    createInputField(registry, 440.0F, 290.0F, InputFieldComponent::ipField("127.0.0.1"), 0);
    createInputField(registry, 440.0F, 430.0F, InputFieldComponent::portField("50010"), 1);

    createButton(registry, 440.0F, 550.0F, "Connect", Color(0, 120, 200), [this]() {
        Logger::instance().info("Connect clicked");
        connecting_ = true;
    });

    createButton(registry, 660.0F, 550.0F, "Settings", Color(70, 70, 70), [this]() {
        Logger::instance().info("Settings clicked");
        done_         = true;
        openSettings_ = true;
    });

    createButton(registry, 1050.0F, 620.0F, "Quit", Color(120, 50, 50), [this]() {
        Logger::instance().info("Quit clicked");
        done_          = true;
        exitRequested_ = true;
    });

    if (!initialError_.empty()) {
        initialError_.clear();
    }
}

void ConnectionMenu::destroy(Registry& registry)
{
    registry.clear();
}

bool ConnectionMenu::isDone() const
{
    return done_;
}

void ConnectionMenu::handleEvent(Registry&, const Event&) {}

void ConnectionMenu::render(Registry& registry, Window&)
{
    if (connecting_) {
        if (connectingText_ == 0 || !registry.isAlive(connectingText_)) {
            showConnectingText(registry);
        }

        constexpr float dotInterval = 0.33F;
        constexpr float exitDelay   = 1.5F;

        auto now      = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - connectingStartTime_).count();

        float cycleTime = dotInterval * 3.0F;
        float phase     = std::fmod(elapsed, cycleTime);
        dotCount_       = (static_cast<int>(phase / dotInterval) % 3) + 1;
        updateConnectingText(registry);

        if (elapsed >= exitDelay) {
            done_ = true;
        }
    }
}

void ConnectionMenu::showConnectingText(Registry& registry)
{
    if (connectingText_ != 0 && registry.isAlive(connectingText_)) {
        return;
    }

    EntityId entity = registry.createEntity();
    auto& transform = registry.emplace<TransformComponent>(entity);
    transform.x     = 550.0F;
    transform.y     = 740.0F;

    auto text          = TextComponent::create("ui", 32, Color(180, 180, 180, 200));
    text.content       = "Connecting to server.";
    text.centered      = true;
    text.centerOffsetY = 10.0F;
    registry.emplace<TextComponent>(entity, text);
    registry.emplace<LayerComponent>(entity, LayerComponent::create(RenderLayer::UI));

    connectingText_      = entity;
    connectingStartTime_ = std::chrono::steady_clock::now();
    dotCount_            = 1;
}

void ConnectionMenu::updateConnectingText(Registry& registry)
{
    if (connectingText_ == 0 || !registry.isAlive(connectingText_)) {
        return;
    }
    if (!registry.has<TextComponent>(connectingText_)) {
        return;
    }

    std::string dots(static_cast<std::size_t>(dotCount_), '.');
    registry.get<TextComponent>(connectingText_).content = "Connecting to server" + dots;
}

ConnectionMenu::Result ConnectionMenu::getResult(Registry& registry) const
{
    Result result;
    result.openSettings  = openSettings_;
    result.exitRequested = exitRequested_;
    result.connected     = done_ && !openSettings_;

    for (EntityId entity : registry.view<InputFieldComponent, FocusableComponent>()) {
        auto& focus = registry.get<FocusableComponent>(entity);
        auto& input = registry.get<InputFieldComponent>(entity);
        if (focus.tabOrder == 0)
            result.ip = input.value;
        else if (focus.tabOrder == 1)
            result.port = input.value;
    }

    return result;
}

void ConnectionMenu::setError(Registry&, const std::string&) {}

void ConnectionMenu::reset()
{
    done_          = false;
    openSettings_  = false;
    exitRequested_ = false;
}
