#include "ui/SettingsMenu.hpp"

#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"

#include <array>
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

SettingsMenu::SettingsMenu(FontManager& fonts, TextureManager& textures, KeyBindings bindings)
    : fonts_(fonts), textures_(textures), currentBindings_(bindings)
{}

void SettingsMenu::create(Registry& registry)
{
    done_ = false;
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

void SettingsMenu::render(Registry&, Window&) {}

SettingsMenu::Result SettingsMenu::getResult(Registry&) const
{
    return Result{currentBindings_};
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
