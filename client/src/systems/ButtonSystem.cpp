#include "systems/ButtonSystem.hpp"

#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"

#include <SFML/Graphics.hpp>

ButtonSystem::ButtonSystem(Window& window, FontManager& fonts) : window_(window), fonts_(fonts) {}

void ButtonSystem::update(Registry& registry, float)
{
    for (EntityId entity : registry.view<TransformComponent, BoxComponent, ButtonComponent>()) {
        if (!registry.isAlive(entity))
            continue;

        auto& transform = registry.get<TransformComponent>(entity);
        auto& box       = registry.get<BoxComponent>(entity);
        auto& button    = registry.get<ButtonComponent>(entity);

        sf::RectangleShape rect(sf::Vector2f{box.width, box.height});
        rect.setPosition(sf::Vector2f{transform.x, transform.y});
        rect.setOutlineThickness(box.outlineThickness);

        if (button.hovered) {
            rect.setFillColor(sf::Color(box.fillColor.r + 30, box.fillColor.g + 30, box.fillColor.b + 30));
            rect.setOutlineColor(box.focusColor);
        } else {
            rect.setFillColor(box.fillColor);
            rect.setOutlineColor(box.outlineColor);
        }

        window_.draw(rect);

        const sf::Font* font = fonts_.get("ui");
        if (font != nullptr && !button.label.empty()) {
            sf::Text text(*font, button.label, 22);
            auto bounds   = text.getLocalBounds();
            float centerX = transform.x + (box.width - bounds.size.x) / 2.0F;
            float centerY = transform.y + (box.height - bounds.size.y) / 2.0F - 5.0F;
            text.setPosition(sf::Vector2f{centerX, centerY});
            text.setFillColor(sf::Color::White);
            window_.draw(text);
        }
    }
}

void ButtonSystem::handleEvent(Registry& registry, const sf::Event& event)
{
    if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseEvent->button == sf::Mouse::Button::Left)
            handleClick(registry, mouseEvent->position.x, mouseEvent->position.y);
    }

    if (const auto* mouseMoveEvent = event.getIf<sf::Event::MouseMoved>())
        handleMouseMove(registry, mouseMoveEvent->position.x, mouseMoveEvent->position.y);
}

void ButtonSystem::handleClick(Registry& registry, int x, int y)
{
    for (EntityId entity : registry.view<TransformComponent, BoxComponent, ButtonComponent>()) {
        auto& transform = registry.get<TransformComponent>(entity);
        auto& box       = registry.get<BoxComponent>(entity);
        auto& button    = registry.get<ButtonComponent>(entity);

        bool inside =
            x >= transform.x && x <= transform.x + box.width && y >= transform.y && y <= transform.y + box.height;
        if (inside && button.onClick != nullptr) {
            button.pressed = true;
            button.onClick();
        }
    }
}

void ButtonSystem::handleMouseMove(Registry& registry, int x, int y)
{
    for (EntityId entity : registry.view<TransformComponent, BoxComponent, ButtonComponent>()) {
        auto& transform = registry.get<TransformComponent>(entity);
        auto& box       = registry.get<BoxComponent>(entity);
        auto& button    = registry.get<ButtonComponent>(entity);

        button.hovered =
            x >= transform.x && x <= transform.x + box.width && y >= transform.y && y <= transform.y + box.height;
    }
}
