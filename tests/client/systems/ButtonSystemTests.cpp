#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/Window.hpp"
#include "systems/ButtonSystem.hpp"

#include <gtest/gtest.h>

class ButtonSystemTest : public ::testing::Test
{
  protected:
    Window window{Vector2u{100, 100}, "Test"};
    FontManager fonts;
    Registry registry;
};

TEST_F(ButtonSystemTest, HoverDetection)
{
    ButtonSystem system(window, fonts);
    EntityId btn = registry.createEntity();
    registry.emplace<TransformComponent>(btn, TransformComponent::create(10.0f, 10.0f));
    registry.emplace<BoxComponent>(btn, BoxComponent::create(50.0f, 20.0f, Color::White, Color::Black));
    auto& button = registry.emplace<ButtonComponent>(btn, ButtonComponent::create("Test", nullptr));

    Event moveEvent;
    moveEvent.type = EventType::MouseMoved;

    moveEvent.mouseMove.x = 0;
    moveEvent.mouseMove.y = 0;
    system.handleEvent(registry, moveEvent);
    EXPECT_FALSE(button.hovered);
    moveEvent.mouseMove.x = 15;
    moveEvent.mouseMove.y = 15;
    system.handleEvent(registry, moveEvent);
    EXPECT_TRUE(button.hovered);
}

TEST_F(ButtonSystemTest, ClickDetection)
{
    ButtonSystem system(window, fonts);
    EntityId btn = registry.createEntity();
    registry.emplace<TransformComponent>(btn, TransformComponent::create(10.0f, 10.0f));
    registry.emplace<BoxComponent>(btn, BoxComponent::create(50.0f, 20.0f, Color::White, Color::Black));

    bool clicked = false;
    auto& button = registry.emplace<ButtonComponent>(btn, ButtonComponent::create("Test", [&]() { clicked = true; }));

    Event clickEvent;
    clickEvent.type               = EventType::MouseButtonPressed;
    clickEvent.mouseButton.button = MouseButton::Left;

    clickEvent.mouseButton.x = 0;
    clickEvent.mouseButton.y = 0;
    system.handleEvent(registry, clickEvent);
    EXPECT_FALSE(clicked);
    EXPECT_FALSE(button.pressed);

    clickEvent.mouseButton.x = 15;
    clickEvent.mouseButton.y = 15;
    system.handleEvent(registry, clickEvent);
    EXPECT_TRUE(clicked);
    EXPECT_TRUE(button.pressed);
}
