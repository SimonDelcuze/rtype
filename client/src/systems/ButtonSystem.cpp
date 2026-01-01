#include "systems/ButtonSystem.hpp"

#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/abstraction/Common.hpp"

ButtonSystem::ButtonSystem(Window& window, FontManager& fonts) : window_(window), fonts_(fonts) {}

void ButtonSystem::update(Registry& registry, float)
{
    for (EntityId entity : registry.view<TransformComponent, BoxComponent, ButtonComponent>()) {
        if (!registry.isAlive(entity))
            continue;

        auto& transform = registry.get<TransformComponent>(entity);
        auto& box       = registry.get<BoxComponent>(entity);
        auto& button    = registry.get<ButtonComponent>(entity);

        auto font = fonts_.get("ui");
        if (font != nullptr && !button.label.empty()) {
            GraphicsFactory factory;
            auto text = factory.createText();
            text->setFont(*font);
            text->setString(button.label);
            text->setCharacterSize(22);

            FloatRect bounds = text->getLocalBounds();
            float centerX    = transform.x + (box.width - bounds.width) / 2.0F;
            float centerY    = transform.y + (box.height - bounds.height) / 2.0F - 5.0F;

            text->setPosition(Vector2f{centerX, centerY});
            text->setFillColor(Color::White);
            window_.draw(*text);
        }
    }
}

void ButtonSystem::handleEvent(Registry& registry, const Event& event)
{
    if (event.type == EventType::MouseButtonPressed) {
        if (event.mouseButton.button == MouseButton::Left)
            handleClick(registry, event.mouseButton.x, event.mouseButton.y);
    }

    if (event.type == EventType::MouseMoved)
        handleMouseMove(registry, event.mouseMove.x, event.mouseMove.y);
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
