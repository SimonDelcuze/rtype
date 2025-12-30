#include "ui/SettingsMenu.hpp"

#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "runtime/MenuMusic.hpp"

#include <SFML/Audio/Listener.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
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

        auto* tex = textures.get("menu_bg");
        if (tex == nullptr)
            return 0;

        EntityId entity  = registry.createEntity();
        auto& transform  = registry.emplace<TransformComponent>(entity);
        transform.x      = 0.0F;
        transform.y      = 0.0F;
        transform.scaleX = 2.25F;
        transform.scaleY = 2.0F;

        SpriteComponent sprite(*tex);
        registry.emplace<SpriteComponent>(entity, sprite);
        return entity;
    }

    EntityId createCenteredText(Registry& registry, float y, const std::string& content)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 640.0F - (content.length() * 36.0F * 0.41F);
        transform.y     = y;

        auto text    = TextComponent::create("ui", 48, sf::Color(220, 220, 220));
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

        auto box       = BoxComponent::create(180.0F, 50.0F, sf::Color(80, 80, 80), sf::Color(120, 120, 120));
        box.focusColor = sf::Color(100, 200, 255);
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

        auto text    = TextComponent::create("ui", 26, sf::Color(220, 220, 220));
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
    startLauncherMusic(musicVolume_);
    if (!fonts_.has("ui"))
        fonts_.load("ui", "client/assets/fonts/ui.ttf");

    awaitingAction_.reset();
    actionButtons_.clear();

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

        sf::Keyboard::Key key;
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

    createCenteredButton(registry, 560.0F + spacing, "Back", [this]() { done_ = true; });
}

void SettingsMenu::destroy(Registry& registry)
{
    registry.clear();
}

bool SettingsMenu::isDone() const
{
    return done_;
}

void SettingsMenu::handleEvent(Registry& registry, const sf::Event& event)
{
    if (const auto* mousePress = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mousePress->button == sf::Mouse::Button::Left)
            handleVolumeMouseEvent(registry, mousePress->position, true);
    }

    if (const auto* mouseRelease = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mouseRelease->button == sf::Mouse::Button::Left)
            draggingVolume_ = false;
    }

    if (const auto* mouseMove = event.getIf<sf::Event::MouseMoved>()) {
        if (draggingVolume_)
            handleVolumeMouseEvent(registry, mouseMove->position, false);
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (awaitingAction_.has_value()) {
            applyBinding(registry, *awaitingAction_, key->code);
            setAwaitingState(registry, *awaitingAction_, false);
            awaitingAction_.reset();
            return;
        }

        if (key->code == sf::Keyboard::Key::Escape)
            done_ = true;
    }
}

void SettingsMenu::render(Registry&, Window& window)
{
    setLauncherMusicVolume(musicVolume_);

    float ratio       = musicVolume_ / 100.0F;
    float filledWidth = sliderWidth_ * std::clamp(ratio, 0.0F, 1.0F);
    sf::RectangleShape track(sf::Vector2f{sliderWidth_, sliderHeight_});
    track.setPosition(sf::Vector2f{sliderX_, sliderY_});
    track.setFillColor(sf::Color(50, 50, 60, 200));
    track.setOutlineThickness(2.0F);
    track.setOutlineColor(sf::Color(80, 120, 160, 200));
    window.draw(track);

    sf::RectangleShape fill(sf::Vector2f{filledWidth, sliderHeight_});
    fill.setPosition(sf::Vector2f{sliderX_, sliderY_});
    fill.setFillColor(sf::Color(90, 190, 255, 230));
    window.draw(fill);

    sf::CircleShape knob(7.0F);
    knob.setOrigin(sf::Vector2f{7.0F, 7.0F});
    knob.setPosition(sf::Vector2f{sliderX_ + filledWidth, sliderY_ + sliderHeight_ / 2.0F});
    knob.setFillColor(sf::Color(200, 230, 255));
    knob.setOutlineColor(sf::Color(40, 120, 200));
    knob.setOutlineThickness(2.0F);
    window.draw(knob);
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

void SettingsMenu::applyBinding(Registry& registry, BindingAction action, sf::Keyboard::Key key)
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

    sf::Keyboard::Key key;
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
    startLauncherMusic(musicVolume_);
    setLauncherMusicVolume(musicVolume_);
    sf::Listener::setGlobalVolume(musicVolume_);
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

bool SettingsMenu::handleVolumeMouseEvent(Registry& registry, const sf::Vector2i& mousePos, bool isClick)
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

std::string SettingsMenu::keyToString(sf::Keyboard::Key key)
{
    switch (key) {
        case sf::Keyboard::Key::Up:
            return "Up";
        case sf::Keyboard::Key::Down:
            return "Down";
        case sf::Keyboard::Key::Left:
            return "Left";
        case sf::Keyboard::Key::Right:
            return "Right";
        case sf::Keyboard::Key::Space:
            return "Space";
        case sf::Keyboard::Key::W:
            return "W";
        case sf::Keyboard::Key::A:
            return "A";
        case sf::Keyboard::Key::S:
            return "S";
        case sf::Keyboard::Key::D:
            return "D";
        case sf::Keyboard::Key::Z:
            return "Z";
        case sf::Keyboard::Key::Q:
            return "Q";
        case sf::Keyboard::Key::E:
            return "E";
        case sf::Keyboard::Key::F:
            return "F";
        default:
            return "Key " + std::to_string(static_cast<int>(key));
    }
}
