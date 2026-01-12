#include "systems/ButtonSystem.hpp"

#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/abstraction/Common.hpp"

ButtonSystem::ButtonSystem(Window& window, FontManager& fonts) : window_(window), fonts_(fonts) {}

void ButtonSystem::update(Registry& registry, float dt)
{
    for (EntityId entity : registry.view<TransformComponent, BoxComponent, ButtonComponent>()) {
        if (!registry.isAlive(entity))
            continue;

        auto& transform = registry.get<TransformComponent>(entity);
        auto& box       = registry.get<BoxComponent>(entity);
        auto& button    = registry.get<ButtonComponent>(entity);

        if (button.autoRepeat) {
            if (button.pressed) {
                button.holdTimer += dt;
                if (button.holdTimer >= button.repeatDelay && button.onClick != nullptr) {
                    float elapsedSinceDelay = button.holdTimer - button.repeatDelay;
                    while (elapsedSinceDelay >= button.repeatInterval) {
                        button.onClick();
                        elapsedSinceDelay -= button.repeatInterval;
                    }
                    button.holdTimer = button.repeatDelay + elapsedSinceDelay;
                }
            } else {
                button.holdTimer = 0.0F;
            }
        } else {
            button.holdTimer = 0.0F;
        }

        auto font = fonts_.get("ui");
        if (font != nullptr && !button.label.empty()) {
            GraphicsFactory factory;
            auto text = factory.createText();
            text->setFont(*font);
            text->setString(button.label);
            text->setCharacterSize(22);

            FloatRect bounds = text->getLocalBounds();
            float centerX    = transform.x + (box.width - bounds.width) / 2.0F + button.textOffsetX;
            float centerY    = transform.y + (box.height - bounds.height) / 2.0F - 5.0F + button.textOffsetY;

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

    if (event.type == EventType::MouseButtonReleased) {
        if (event.mouseButton.button == MouseButton::Left) {
            for (EntityId entity : registry.view<ButtonComponent>()) {
                auto& btn     = registry.get<ButtonComponent>(entity);
                btn.pressed   = false;
                btn.holdTimer = 0.0F;
            }
        }
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
