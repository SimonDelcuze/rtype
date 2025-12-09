#include "systems/InputFieldSystem.hpp"

#include "components/BoxComponent.hpp"
#include "components/FocusableComponent.hpp"
#include "components/InputFieldComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>

InputFieldSystem::InputFieldSystem(Window& window, FontManager& fonts) : window_(window), fonts_(fonts) {}

void InputFieldSystem::update(Registry& registry, float)
{
    for (EntityId entity : registry.view<TransformComponent, BoxComponent, InputFieldComponent>()) {
        if (!registry.isAlive(entity))
            continue;

        auto& transform = registry.get<TransformComponent>(entity);
        auto& box       = registry.get<BoxComponent>(entity);
        auto& input     = registry.get<InputFieldComponent>(entity);

        sf::RectangleShape rect(sf::Vector2f{box.width, box.height});
        rect.setPosition(sf::Vector2f{transform.x, transform.y});
        rect.setFillColor(box.fillColor);
        rect.setOutlineThickness(box.outlineThickness);
        rect.setOutlineColor(input.focused ? box.focusColor : box.outlineColor);
        window_.draw(rect);

        const sf::Font* font = fonts_.get("ui");
        if (font == nullptr)
            continue;

        std::string displayText = input.value;
        if (input.focused)
            displayText += "_";
        else if (input.value.empty() && !input.placeholder.empty())
            displayText = input.placeholder;

        sf::Text text(*font, displayText, 20);
        text.setPosition(sf::Vector2f{transform.x + 10.0F, transform.y + 13.0F});
        text.setFillColor(sf::Color::White);
        window_.draw(text);
    }
}

void InputFieldSystem::handleEvent(Registry& registry, const sf::Event& event)
{
    if (const auto* textEvent = event.getIf<sf::Event::TextEntered>()) {
        char c = static_cast<char>(textEvent->unicode);
        if (c >= 32 && c < 127)
            handleTextEntered(registry, c);
    }

    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Backspace)
            handleBackspace(registry);
        else if (keyEvent->code == sf::Keyboard::Key::Tab)
            handleTab(registry);
    }

    if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseEvent->button == sf::Mouse::Button::Left)
            handleClick(registry, mouseEvent->position.x, mouseEvent->position.y);
    }
}

void InputFieldSystem::handleTextEntered(Registry& registry, char c)
{
    for (EntityId entity : registry.view<InputFieldComponent>()) {
        auto& input = registry.get<InputFieldComponent>(entity);
        if (!input.focused || input.value.length() >= input.maxLength)
            continue;
        if (input.validator != nullptr && !input.validator(c))
            continue;
        input.value += c;
    }
}

void InputFieldSystem::handleBackspace(Registry& registry)
{
    for (EntityId entity : registry.view<InputFieldComponent>()) {
        auto& input = registry.get<InputFieldComponent>(entity);
        if (input.focused && !input.value.empty())
            input.value.pop_back();
    }
}

void InputFieldSystem::handleTab(Registry& registry)
{
    std::vector<std::pair<EntityId, int>> focusables;
    for (EntityId entity : registry.view<InputFieldComponent, FocusableComponent>()) {
        focusables.emplace_back(entity, registry.get<FocusableComponent>(entity).tabOrder);
    }

    if (focusables.empty())
        return;

    std::sort(focusables.begin(), focusables.end(), [](const auto& a, const auto& b) { return a.second < b.second; });

    int currentIdx = -1;
    for (std::size_t i = 0; i < focusables.size(); ++i) {
        auto& input = registry.get<InputFieldComponent>(focusables[i].first);
        if (input.focused) {
            currentIdx                                                    = static_cast<int>(i);
            input.focused                                                 = false;
            registry.get<FocusableComponent>(focusables[i].first).focused = false;
        }
    }

    int nextIdx = (currentIdx + 1) % static_cast<int>(focusables.size());
    registry.get<InputFieldComponent>(focusables[nextIdx].first).focused = true;
    registry.get<FocusableComponent>(focusables[nextIdx].first).focused  = true;
}

void InputFieldSystem::handleClick(Registry& registry, int x, int y)
{
    for (EntityId entity : registry.view<TransformComponent, BoxComponent, InputFieldComponent>()) {
        auto& transform = registry.get<TransformComponent>(entity);
        auto& box       = registry.get<BoxComponent>(entity);
        auto& input     = registry.get<InputFieldComponent>(entity);

        bool inside =
            x >= transform.x && x <= transform.x + box.width && y >= transform.y && y <= transform.y + box.height;
        input.focused = inside;

        if (registry.has<FocusableComponent>(entity))
            registry.get<FocusableComponent>(entity).focused = inside;
    }
}
