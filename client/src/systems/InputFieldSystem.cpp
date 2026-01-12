#include "systems/InputFieldSystem.hpp"

#include "components/BoxComponent.hpp"
#include "components/FocusableComponent.hpp"
#include "components/InputFieldComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/abstraction/Common.hpp"

#include <algorithm>
#include <string>

InputFieldSystem::InputFieldSystem(Window& window, FontManager& fonts) : window_(window), fonts_(fonts) {}

void InputFieldSystem::update(Registry& registry, float)
{
    for (EntityId entity : registry.view<TransformComponent, BoxComponent, InputFieldComponent>()) {
        if (!registry.isAlive(entity))
            continue;

        auto& transform = registry.get<TransformComponent>(entity);
        auto& input     = registry.get<InputFieldComponent>(entity);
        auto& box       = registry.get<BoxComponent>(entity);

        auto font = fonts_.get("ui");
        if (font == nullptr)
            continue;

        if (!input.editable) {
            input.focused = false;
        }

        std::string displayText = input.value;

        if (input.passwordField && !input.value.empty()) {
            displayText = std::string(input.value.length(), '*');
        }

        if (input.focused)
            displayText += "_";
        else if (input.value.empty() && !input.placeholder.empty())
            displayText = input.placeholder;

        GraphicsFactory factory;
        auto text = factory.createText();
        text->setFont(*font);
        text->setCharacterSize(20);
        text->setFillColor(Color::White);

        std::string fullText = displayText;
        float maxWidth       = box.width - 2.0F * input.paddingX;

        text->setString(fullText);
        if (text->getGlobalBounds().width > maxWidth) {
            std::string visibleText = fullText;
            while (!visibleText.empty() && text->getGlobalBounds().width > maxWidth) {
                visibleText.erase(0, 1);
                text->setString(visibleText);
            }
        }

        float padX = input.paddingX;
        if (input.centerVertically) {
            auto bounds    = text->getLocalBounds();
            float textH    = bounds.height;
            float baseline = bounds.top;
            text->setPosition(Vector2f{transform.x + padX, transform.y + (box.height - textH) * 0.5F - baseline});
        } else {
            text->setPosition(Vector2f{transform.x + padX, transform.y + input.paddingY});
        }
        window_.draw(*text);
    }
}

void InputFieldSystem::handleEvent(Registry& registry, const Event& event)
{
    if (event.type == EventType::TextEntered) {
        char c = static_cast<char>(event.text.unicode);
        if (c >= 32 && c < 127)
            handleTextEntered(registry, c);
    }

    if (event.type == EventType::KeyPressed) {
        if (event.key.code == KeyCode::Backspace)
            handleBackspace(registry);
        else if (event.key.code == KeyCode::Tab)
            handleTab(registry);
    }

    if (event.type == EventType::MouseButtonPressed) {
        if (event.mouseButton.button == MouseButton::Left)
            handleClick(registry, event.mouseButton.x, event.mouseButton.y);
    }
}

void InputFieldSystem::handleTextEntered(Registry& registry, char c)
{
    for (EntityId entity : registry.view<InputFieldComponent>()) {
        auto& input = registry.get<InputFieldComponent>(entity);
        if (!input.editable || !input.focused || input.value.length() >= input.maxLength)
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
        if (!input.editable)
            continue;
        if (input.focused && !input.value.empty())
            input.value.pop_back();
    }
}

void InputFieldSystem::handleTab(Registry& registry)
{
    std::vector<std::pair<EntityId, int>> focusables;
    for (EntityId entity : registry.view<InputFieldComponent, FocusableComponent>()) {
        if (!registry.get<InputFieldComponent>(entity).editable)
            continue;
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

        if (!input.editable) {
            input.focused = false;
            if (registry.has<FocusableComponent>(entity))
                registry.get<FocusableComponent>(entity).focused = false;
            continue;
        }

        bool inside =
            x >= transform.x && x <= transform.x + box.width && y >= transform.y && y <= transform.y + box.height;
        input.focused = inside;

        if (registry.has<FocusableComponent>(entity))
            registry.get<FocusableComponent>(entity).focused = inside;
    }
}
