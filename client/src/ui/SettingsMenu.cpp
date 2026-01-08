#include "ui/SettingsMenu.hpp"

#include "ClientRuntime.hpp"
#include "audio/SoundManager.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "input/KeyBindings.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <unordered_map>

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

        return entity;
    }

    EntityId createCenteredText(Registry& registry, float y, const std::string& content)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 640.0F - (content.length() * 36.0F * 0.41F);
        transform.y     = y;

        auto text    = TextComponent::create("ui", 48, Color(220, 220, 220));
        text.content = content;
        registry.emplace<TextComponent>(entity, text);
        return entity;
    }

    EntityId createCenteredButton(Registry& registry, float y, const std::string& label, std::function<void()> onClick)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 550.0F;
        transform.y     = y;

        auto box       = BoxComponent::create(180.0F, 50.0F, Color(80, 80, 80), Color(120, 120, 120));
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        return entity;
    }

    EntityId createLabel(Registry& registry, float x, float y, const std::string& content)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto text    = TextComponent::create("ui", 26, Color(220, 220, 220));
        text.content = content;
        registry.emplace<TextComponent>(entity, text);
        return entity;
    }
} // namespace

SettingsMenu::SettingsMenu(FontManager& fonts, TextureManager& textures, KeyBindings bindings, float musicVolume)
    : fonts_(fonts), textures_(textures), currentBindings_(bindings),
      musicVolume_(std::clamp(musicVolume, 0.0F, 100.0F))
{}

void SettingsMenu::create(Registry& registry)
{
    done_           = false;
    draggingVolume_ = false;
    if (!fonts_.has("ui"))
        fonts_.load("ui", "client/assets/fonts/ui.ttf");

    awaitingAction_.reset();
    actionButtons_.clear();
    originalPositions_.clear();

    createBackground(registry, textures_);
    createCenteredText(registry, 40.0F, "SETTINGS");

    float startY                                                = 150.0F;
    float spacing                                               = 80.0F;
    std::array<std::pair<BindingAction, std::string>, 5> labels = {{
        {BindingAction::Up, "Move Up"},
        {BindingAction::Down, "Move Down"},
        {BindingAction::Left, "Move Left"},
        {BindingAction::Right, "Move Right"},
        {BindingAction::Fire, "Fire"},
    }};

    int idx = 0;
    for (const auto& pair : labels) {
        const BindingAction action = pair.first;
        const std::string& label   = pair.second;
        float y                    = startY + spacing * static_cast<float>(idx++);

        createLabel(registry, 360.0F, y + 12.0F, label);

        KeyCode key = KeyCode::Unknown;
        switch (action) {
            case BindingAction::Up:
                key = currentBindings_.up;
                break;
            case BindingAction::Down:
                key = currentBindings_.down;
                break;
            case BindingAction::Left:
                key = currentBindings_.left;
                break;
            case BindingAction::Right:
                key = currentBindings_.right;
                break;
            case BindingAction::Fire:
                key = currentBindings_.fire;
                break;
        }

        EntityId buttonId      = createCenteredButton(registry, y, keyToString(key),
                                                      [this, action, &registry]() { startRebinding(registry, action); });
        actionButtons_[action] = buttonId;
    }

    float sliderRowY = startY + spacing * static_cast<float>(labels.size());
    sliderY_         = sliderRowY + 16.0F;
    sliderWidth_     = 160.0F;
    sliderHeight_    = 6.0F;
    sliderX_         = 640.0F - sliderWidth_ / 2.0F;

    createLabel(registry, 360.0F, sliderRowY, "Volume");
    volumeValueLabel_ = createLabel(registry, sliderX_ + sliderWidth_ + 24.0F, sliderRowY, "");
    refreshVolumeLabel(registry);

    float networkDebugY = sliderRowY + spacing;
    createLabel(registry, 360.0F, networkDebugY + 12.0F, "Network Debug");
    networkDebugButton_ = createCenteredButton(registry, networkDebugY, g_networkDebugEnabled ? "ON" : "OFF", [this, &registry]() {
        g_networkDebugEnabled = !g_networkDebugEnabled;
        if (registry.isAlive(networkDebugButton_) && registry.has<ButtonComponent>(networkDebugButton_)) {
            registry.get<ButtonComponent>(networkDebugButton_).label = g_networkDebugEnabled ? "ON" : "OFF";
        }
    });

    float backButtonY = networkDebugY + spacing + 40.0F;
    createCenteredButton(registry, backButtonY, "Back", [this]() { done_ = true; });

    contentHeight_ = backButtonY + 60.0F;
    scrollOffset_ = 0.0F;
    applyScrollOffset(registry);
}

void SettingsMenu::destroy(Registry& registry)
{
    registry.clear();
}

bool SettingsMenu::isDone() const
{
    return done_;
}

void SettingsMenu::handleEvent(Registry& registry, const Event& event)
{
    if (event.type == EventType::MouseButtonPressed) {
        if (event.mouseButton.button == MouseButton::Left)
            handleVolumeMouseEvent(registry, Vector2i{event.mouseButton.x, event.mouseButton.y}, true);
    }

    if (event.type == EventType::MouseButtonReleased) {
        if (event.mouseButton.button == MouseButton::Left)
            draggingVolume_ = false;
    }

    if (event.type == EventType::MouseMoved) {
        if (draggingVolume_)
            handleVolumeMouseEvent(registry, Vector2i{event.mouseMove.x, event.mouseMove.y}, false);
    }

    if (event.type == EventType::MouseWheelScrolled) {
        constexpr float scrollSpeed = 30.0F;
        float windowHeight = 720.0F;
        float maxScroll = std::max(0.0F, contentHeight_ - windowHeight + 100.0F);

        scrollOffset_ -= event.mouseWheelScroll.delta * scrollSpeed;
        scrollOffset_ = std::clamp(scrollOffset_, 0.0F, maxScroll);
        applyScrollOffset(registry);
    }

    if (event.type == EventType::KeyPressed) {
        if (awaitingAction_.has_value()) {
            applyBinding(registry, *awaitingAction_, event.key.code);
        }

        if (event.key.code == KeyCode::Escape)
            done_ = true;

        constexpr float scrollSpeed = 30.0F;
        float windowHeight = 720.0F;
        float maxScroll = std::max(0.0F, contentHeight_ - windowHeight + 100.0F);

        if (event.key.code == KeyCode::Up) {
            scrollOffset_ = std::max(0.0F, scrollOffset_ - scrollSpeed);
            applyScrollOffset(registry);
        } else if (event.key.code == KeyCode::Down) {
            scrollOffset_ = std::min(maxScroll, scrollOffset_ + scrollSpeed);
            applyScrollOffset(registry);
        }
    }
}

void SettingsMenu::render(Registry&, Window& window)
{
    float ratio       = std::clamp(musicVolume_ / 100.0F, 0.0F, 1.0F);
    float filledWidth = sliderWidth_ * ratio;

    window.drawRectangle({sliderWidth_, sliderHeight_}, {sliderX_, sliderY_}, 0.0F, {1.0F, 1.0F}, Color(60, 60, 60),
                         Color::Transparent, 1.0F);

    if (filledWidth > 0.0F) {
        window.drawRectangle({filledWidth, sliderHeight_}, {sliderX_, sliderY_}, 0.0F, {1.0F, 1.0F}, Color(0, 180, 255),
                             Color::Transparent, 0.0F);
    }

    float knobSize = 16.0F;
    float knobX    = sliderX_ + filledWidth - (knobSize / 2.0F);
    float knobY    = sliderY_ + (sliderHeight_ / 2.0F) - (knobSize / 2.0F);
    window.drawRectangle({knobSize, knobSize}, {knobX, knobY}, 0.0F, {1.0F, 1.0F}, Color::White, Color(0, 100, 200),
                         2.0F);
}

SettingsMenu::Result SettingsMenu::getResult(Registry&) const
{
    return Result{currentBindings_, musicVolume_};
}

void SettingsMenu::startRebinding(Registry& registry, BindingAction action)
{
    if (awaitingAction_.has_value())
        setAwaitingState(registry, *awaitingAction_, false);
    awaitingAction_ = action;
    setAwaitingState(registry, action, true);
}

void SettingsMenu::applyBinding(Registry& registry, BindingAction action, KeyCode key)
{
    switch (action) {
        case BindingAction::Up:
            currentBindings_.up = key;
            break;
        case BindingAction::Down:
            currentBindings_.down = key;
            break;
        case BindingAction::Left:
            currentBindings_.left = key;
            break;
        case BindingAction::Right:
            currentBindings_.right = key;
            break;
        case BindingAction::Fire:
            currentBindings_.fire = key;
            break;
    }

    refreshButtonLabel(registry, action);
}

void SettingsMenu::setAwaitingState(Registry& registry, BindingAction action, bool awaiting)
{
    auto it = actionButtons_.find(action);
    if (it == actionButtons_.end())
        return;
    if (!registry.isAlive(it->second) || !registry.has<ButtonComponent>(it->second))
        return;

    auto& button = registry.get<ButtonComponent>(it->second);
    if (awaiting) {
        button.label   = "Press key...";
        button.hovered = true;
    } else {
        button.hovered = false;
        refreshButtonLabel(registry, action);
    }
}

void SettingsMenu::refreshButtonLabel(Registry& registry, BindingAction action)
{
    auto it = actionButtons_.find(action);
    if (it == actionButtons_.end())
        return;
    if (!registry.isAlive(it->second) || !registry.has<ButtonComponent>(it->second))
        return;

    KeyCode key = KeyCode::Unknown;
    switch (action) {
        case BindingAction::Up:
            key = currentBindings_.up;
            break;
        case BindingAction::Down:
            key = currentBindings_.down;
            break;
        case BindingAction::Left:
            key = currentBindings_.left;
            break;
        case BindingAction::Right:
            key = currentBindings_.right;
            break;
        case BindingAction::Fire:
            key = currentBindings_.fire;
            break;
    }

    auto& button = registry.get<ButtonComponent>(it->second);
    button.label = keyToString(key);
}

void SettingsMenu::setMusicVolume(Registry& registry, float volume)
{
    musicVolume_ = std::clamp(volume, 0.0F, 100.0F);
    SoundManager::setGlobalVolume(musicVolume_);
    refreshVolumeLabel(registry);
}

void SettingsMenu::refreshVolumeLabel(Registry& registry)
{
    if (volumeValueLabel_ == 0 || !registry.isAlive(volumeValueLabel_) ||
        !registry.has<TextComponent>(volumeValueLabel_)) {
        return;
    }

    auto& text   = registry.get<TextComponent>(volumeValueLabel_);
    text.content = std::to_string(static_cast<int>(std::lround(std::clamp(musicVolume_, 0.0F, 100.0F)))) + "%";
}

bool SettingsMenu::handleVolumeMouseEvent(Registry& registry, const Vector2i& mousePos, bool isClick)
{
    float minX = sliderX_;
    float maxX = sliderX_ + sliderWidth_;
    float minY = sliderY_ - 12.0F;
    float maxY = sliderY_ + sliderHeight_ + 20.0F;

    bool inside = static_cast<float>(mousePos.x) >= minX && static_cast<float>(mousePos.x) <= maxX &&
                  static_cast<float>(mousePos.y) >= minY && static_cast<float>(mousePos.y) <= maxY;

    if (isClick) {
        if (!inside)
            return false;
        draggingVolume_ = true;
    }

    if (!draggingVolume_)
        return false;

    float ratio = static_cast<float>(mousePos.x - sliderX_) / sliderWidth_;
    ratio       = std::clamp(ratio, 0.0F, 1.0F);
    setMusicVolume(registry, ratio * 100.0F);
    return true;
}

void SettingsMenu::applyScrollOffset(Registry& registry)
{
    if (originalPositions_.empty()) {
        for (EntityId id = 0; id < registry.entityCount(); ++id) {
            if (registry.isAlive(id) && registry.has<TransformComponent>(id)) {
                originalPositions_[id] = registry.get<TransformComponent>(id).y;
            }
        }
        baseSliderY_ = sliderY_;
    }

    for (const auto& [id, originalY] : originalPositions_) {
        if (id == 0) {
            continue;
        }
        if (registry.isAlive(id) && registry.has<TransformComponent>(id)) {
            registry.get<TransformComponent>(id).y = originalY - scrollOffset_;
        }
    }

    sliderY_ = baseSliderY_ - scrollOffset_;
}

std::string SettingsMenu::keyToString(KeyCode code)
{
    switch (code) {
        case KeyCode::Up:
            return "Up";
        case KeyCode::Down:
            return "Down";
        case KeyCode::Left:
            return "Left";
        case KeyCode::Right:
            return "Right";
        case KeyCode::Space:
            return "Space";
        case KeyCode::W:
            return "W";
        case KeyCode::A:
            return "A";
        case KeyCode::S:
            return "S";
        case KeyCode::D:
            return "D";
        case KeyCode::Z:
            return "Z";
        case KeyCode::Q:
            return "Q";
        case KeyCode::E:
            return "E";
        case KeyCode::F:
            return "F";
        case KeyCode::Enter:
            return "Enter";
        case KeyCode::Escape:
            return "Escape";
        default:
            return "Key(" + std::to_string(static_cast<int>(code)) + ")";
    }
}
