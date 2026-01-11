# ButtonComponent

## Overview

The `ButtonComponent` is an essential UI component in the R-Type client architecture that enables interactive button functionality. It provides the data and state management necessary for creating clickable UI elements throughout the game's interface, including menus, dialogs, and in-game UI elements.

## Purpose and Design Philosophy

Interactive buttons are fundamental to any game's user interface. The `ButtonComponent` implements a clean, event-driven approach to button interaction:

1. **State Tracking**: Monitors hover and pressed states for visual feedback
2. **Callback System**: Uses `std::function` for flexible click handling
3. **Label Management**: Stores display text for rendering
4. **ECS Integration**: Works seamlessly with the Entity Component System

The component follows the data-oriented design principle, storing only the state and callback reference. All visual updates and interaction detection are handled by the `ButtonSystem`.

## Component Structure

```cpp
struct ButtonComponent
{
    std::string label;
    bool hovered                  = false;
    bool pressed                  = false;
    std::function<void()> onClick = nullptr;

    static ButtonComponent create(const std::string& text, std::function<void()> callback);
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `label` | `std::string` | `""` | The text displayed on the button. |
| `hovered` | `bool` | `false` | Whether the mouse cursor is currently over the button. |
| `pressed` | `bool` | `false` | Whether the button is currently being pressed. |
| `onClick` | `std::function<void()>` | `nullptr` | The callback function invoked when the button is clicked. |

## Factory Method

### `create(const std::string& text, std::function<void()> callback)`

Creates a new `ButtonComponent` with the specified label and click handler.

**Parameters:**
- `text`: The display text for the button
- `callback`: The function to call when the button is clicked

**Returns:** A configured `ButtonComponent` instance.

**Example:**
```cpp
// Create a play button
auto playBtn = ButtonComponent::create("Play Game", [this]() {
    startGame();
});

// Create a quit button with lambda
auto quitBtn = ButtonComponent::create("Quit", []() {
    std::exit(0);
});
```

## Reset Value Support

The `ButtonComponent` implements the `resetValue` template specialization:

```cpp
template <> inline void resetValue<ButtonComponent>(ButtonComponent& value)
{
    value = ButtonComponent{};
}
```

This ensures proper cleanup when component pooling recycles button entities.

## Usage Patterns

### Basic Menu Button

Creating a standard menu button:

```cpp
Entity createMenuButton(Registry& registry, float x, float y, 
                        const std::string& label, std::function<void()> onClick,
                        int tabOrder)
{
    auto entity = registry.createEntity();
    
    // Position
    registry.addComponent(entity, TransformComponent::create(x, y));
    
    // Visual background
    auto box = BoxComponent::create(
        250.0F, 55.0F,
        Color{45, 45, 60, 255},
        Color{90, 90, 120, 255}
    );
    box.focusColor = Color{100, 180, 255, 255};
    registry.addComponent(entity, box);
    
    // Button behavior
    registry.addComponent(entity, ButtonComponent::create(label, onClick));
    
    // Text display
    auto text = TextComponent::create(label, 22);
    text.color = Color{220, 220, 230, 255};
    registry.addComponent(entity, text);
    
    // Keyboard/gamepad navigation support
    registry.addComponent(entity, FocusableComponent::create(tabOrder));
    
    // UI layer
    registry.addComponent(entity, LayerComponent::create(RenderLayer::UI));
    
    return entity;
}
```

### Creating a Menu with Multiple Buttons

```cpp
void MainMenu::createButtons(Registry& registry)
{
    const float startY = 300.0F;
    const float buttonSpacing = 70.0F;
    const float centerX = SCREEN_WIDTH / 2 - 125.0F;
    
    // Play Button
    createMenuButton(registry, centerX, startY, "Play", [this]() {
        m_menuRunner.switchTo(MenuState::LobbyConnect);
    }, 0);
    
    // Settings Button
    createMenuButton(registry, centerX, startY + buttonSpacing, "Settings", [this]() {
        m_menuRunner.switchTo(MenuState::Settings);
    }, 1);
    
    // Profile Button
    createMenuButton(registry, centerX, startY + buttonSpacing * 2, "Profile", [this]() {
        m_menuRunner.switchTo(MenuState::Profile);
    }, 2);
    
    // Quit Button
    createMenuButton(registry, centerX, startY + buttonSpacing * 3, "Quit", [this]() {
        m_window.close();
    }, 3);
}
```

### Icon Button

Button with icon instead of text:

```cpp
Entity createIconButton(Registry& registry, float x, float y, 
                        const std::string& iconSprite, std::function<void()> onClick)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    
    // Square background for icon
    registry.addComponent(entity, BoxComponent::create(
        48.0F, 48.0F,
        Color{50, 50, 65, 255},
        Color{80, 80, 100, 255}
    ));
    
    // Icon sprite
    registry.addComponent(entity, SpriteComponent::create(iconSprite));
    
    // Button behavior (empty label for icon buttons)
    registry.addComponent(entity, ButtonComponent::create("", onClick));
    
    registry.addComponent(entity, FocusableComponent::create(0));
    registry.addComponent(entity, LayerComponent::create(RenderLayer::UI));
    
    return entity;
}
```

### Toggle Button

Button that toggles state:

```cpp
Entity createToggleButton(Registry& registry, float x, float y,
                          const std::string& label, bool& toggleState)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    
    // Use color to indicate state
    Color fillColor = toggleState ? Color{60, 120, 80} : Color{50, 50, 65};
    registry.addComponent(entity, BoxComponent::create(200.0F, 45.0F, fillColor, Color{90, 90, 110}));
    
    // Button that toggles the state
    registry.addComponent(entity, ButtonComponent::create(label, [&toggleState, entity, &registry]() {
        toggleState = !toggleState;
        
        // Update visual
        auto& box = registry.getComponent<BoxComponent>(entity);
        box.fillColor = toggleState ? Color{60, 120, 80} : Color{50, 50, 65};
    }));
    
    registry.addComponent(entity, TextComponent::create(label, 18));
    registry.addComponent(entity, FocusableComponent::create(0));
    registry.addComponent(entity, LayerComponent::create(RenderLayer::UI));
    
    return entity;
}
```

## Integration with ButtonSystem

The `ButtonSystem` processes button entities each frame:

```cpp
void ButtonSystem::update(Registry& registry, Window& window, float deltaTime)
{
    auto mousePos = window.getMousePosition();
    bool mousePressed = window.isMouseButtonPressed(MouseButton::Left);
    
    auto view = registry.view<ButtonComponent, BoxComponent, TransformComponent>();
    
    for (auto entity : view) {
        auto& button = view.get<ButtonComponent>(entity);
        auto& box = view.get<BoxComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        
        // Check if mouse is over button
        bool wasHovered = button.hovered;
        button.hovered = isPointInRect(mousePos, transform, box);
        
        // Play hover sound on enter
        if (button.hovered && !wasHovered) {
            playSound("ui_hover");
        }
        
        // Track pressed state
        bool wasPressed = button.pressed;
        button.pressed = button.hovered && mousePressed;
        
        // Trigger click on release
        if (wasPressed && !button.pressed && button.hovered) {
            playSound("ui_click");
            if (button.onClick) {
                button.onClick();
            }
        }
    }
}
```

## Keyboard/Gamepad Support

Buttons work with `FocusableComponent` for keyboard navigation:

```cpp
void FocusNavigationSystem::handleInput(Registry& registry, const Event& event)
{
    if (event.type == EventType::KeyPressed) {
        if (event.key == Key::Enter || event.key == Key::Space) {
            // Find focused button and trigger click
            auto view = registry.view<ButtonComponent, FocusableComponent>();
            
            for (auto entity : view) {
                auto& focus = view.get<FocusableComponent>(entity);
                auto& button = view.get<ButtonComponent>(entity);
                
                if (focus.focused && button.onClick) {
                    playSound("ui_click");
                    button.onClick();
                }
            }
        }
    }
}
```

## Visual Feedback States

Buttons typically have multiple visual states:

| State | Description | Visual Treatment |
|-------|-------------|------------------|
| **Normal** | Default state | Base colors |
| **Hovered** | Mouse is over button | Lighter fill, brighter outline |
| **Pressed** | Button is being clicked | Darker fill, offset position |
| **Focused** | Keyboard focus active | Focus color outline |
| **Disabled** | Button cannot be clicked | Grayed out, no interaction |

## Animation Integration

For animated button effects:

```cpp
void animateButtonHover(Registry& registry, Entity entity, bool entering)
{
    auto& box = registry.getComponent<BoxComponent>(entity);
    
    if (entering) {
        // Scale up slightly and brighten
        box.fillColor = Color{
            std::min(255, box.fillColor.r + 20),
            std::min(255, box.fillColor.g + 20),
            std::min(255, box.fillColor.b + 30)
        };
    } else {
        // Return to normal
        box.fillColor = Color{45, 45, 60};
    }
}
```

## Best Practices

1. **Consistent Sizing**: Use standard button sizes for visual coherence
2. **Clear Labels**: Use short, action-oriented text ("Play", "Save", "Back")
3. **Callback Safety**: Ensure callbacks don't invalidate iterators
4. **Tab Order**: Assign logical tab orders for keyboard navigation
5. **Audio Feedback**: Always provide audio feedback on hover/click
6. **Visual Feedback**: Make state changes clearly visible

## Performance Considerations

- **Callback Storage**: `std::function` has some overhead; avoid in hot paths
- **State Checks**: Only process buttons in UI layer during menu scenes
- **Mouse Detection**: Use spatial partitioning for many buttons

## Accessibility

- **Focus Indicators**: Ensure focused state is clearly visible
- **Keyboard Navigation**: All buttons should be reachable via Tab
- **Gamepad Support**: Map A/X button to confirm
- **Clear Labels**: Use descriptive, readable text

## Related Components

- [BoxComponent](box-component.md) - Visual background
- [TextComponent](text-component.md) - Button label display
- [FocusableComponent](focusable-component.md) - Keyboard navigation
- [LayerComponent](layer-component.md) - Render ordering

## Related Systems

- [ButtonSystem](../systems/button-system.md) - Handles button interaction
- [RenderSystem](../systems/render-system.md) - Visual rendering
- [AudioSystem](../systems/audio-system.md) - Click/hover sounds
