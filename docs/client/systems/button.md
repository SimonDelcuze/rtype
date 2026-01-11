# ButtonSystem

## Overview

The `ButtonSystem` is a core UI interaction system in the R-Type client that manages all button-related behaviors including mouse hover detection, click handling, and visual state updates. It processes user input events and coordinates with the rendering system to provide responsive, visually consistent button interactions throughout the game's menu interfaces.

## Purpose and Design Philosophy

Interactive buttons require coordinated handling of multiple concerns:

1. **Hit Detection**: Determining when mouse is over a button
2. **State Management**: Tracking hover and pressed states
3. **Click Triggering**: Invoking callbacks when clicks occur
4. **Visual Feedback**: Coordinating with render for hover/pressed states
5. **Audio Feedback**: Playing appropriate sounds

The system follows the single-responsibility principle by focusing solely on button logic, delegating rendering to `RenderSystem` and audio to `AudioSystem`.

## Class Definition

```cpp
class ButtonSystem : public ISystem
{
  public:
    ButtonSystem(Window& window, FontManager& fonts);

    void update(Registry& registry, float deltaTime) override;
    void handleEvent(Registry& registry, const Event& event);

  private:
    void handleClick(Registry& registry, int x, int y);
    void handleMouseMove(Registry& registry, int x, int y);

    Window& window_;
    FontManager& fonts_;
};
```

## Constructor

```cpp
ButtonSystem(Window& window, FontManager& fonts);
```

**Parameters:**
- `window`: Reference to the game window for mouse position queries
- `fonts`: Reference to font manager for text measurement

**Example:**
```cpp
ButtonSystem buttonSystem(window, fontManager);
scheduler.addSystem(&buttonSystem);
```

## Methods

### `update(Registry& registry, float deltaTime)`

Called each frame to update button states based on current mouse position.

```cpp
void update(Registry& registry, float deltaTime) override
{
    // Get current mouse position
    auto mousePos = window_.getMousePosition();
    
    // Update hover states for all buttons
    handleMouseMove(registry, mousePos.x, mousePos.y);
}
```

### `handleEvent(Registry& registry, const Event& event)`

Processes input events for button interactions.

```cpp
void handleEvent(Registry& registry, const Event& event)
{
    if (event.type == EventType::MouseButtonPressed && 
        event.mouseButton == MouseButton::Left) {
        handleClick(registry, event.mouseX, event.mouseY);
    }
}
```

### `handleClick(Registry& registry, int x, int y)`

Detects and triggers button clicks.

```cpp
void handleClick(Registry& registry, int x, int y)
{
    auto view = registry.view<ButtonComponent, BoxComponent, TransformComponent>();
    
    for (auto entity : view) {
        auto& button = view.get<ButtonComponent>(entity);
        auto& box = view.get<BoxComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        
        if (isPointInButton(x, y, transform, box)) {
            button.pressed = true;
            
            if (button.onClick) {
                playClickSound();
                button.onClick();
            }
        }
    }
}
```

### `handleMouseMove(Registry& registry, int x, int y)`

Updates hover states for all buttons.

```cpp
void handleMouseMove(Registry& registry, int x, int y)
{
    auto view = registry.view<ButtonComponent, BoxComponent, TransformComponent>();
    
    for (auto entity : view) {
        auto& button = view.get<ButtonComponent>(entity);
        auto& box = view.get<BoxComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        
        bool wasHovered = button.hovered;
        button.hovered = isPointInButton(x, y, transform, box);
        
        // Play hover sound on enter
        if (button.hovered && !wasHovered) {
            playHoverSound();
        }
    }
}
```

## Hit Detection Logic

```cpp
bool ButtonSystem::isPointInButton(int x, int y, 
                                    const TransformComponent& transform,
                                    const BoxComponent& box)
{
    float left = transform.x;
    float top = transform.y;
    float right = transform.x + box.width;
    float bottom = transform.y + box.height;
    
    return x >= left && x <= right && y >= top && y <= bottom;
}
```

## Integration with Game Loop

```cpp
void ClientLoop::handleEvents()
{
    Event event;
    while (window_.pollEvent(event)) {
        // Pass events to button system
        buttonSystem_.handleEvent(registry_, event);
        
        // Also pass to input field system
        inputFieldSystem_.handleEvent(registry_, event);
    }
}

void ClientLoop::update(float deltaTime)
{
    // Button system updates hover states
    buttonSystem_.update(registry_, deltaTime);
    
    // ... other systems
}
```

## Focus Integration

Buttons can be activated via keyboard when focused:

```cpp
void ButtonSystem::handleKeyboardActivation(Registry& registry)
{
    auto view = registry.view<ButtonComponent, FocusableComponent>();
    
    for (auto entity : view) {
        auto& button = view.get<ButtonComponent>(entity);
        auto& focus = view.get<FocusableComponent>(entity);
        
        if (focus.focused && keyboard_.isKeyJustPressed(Key::Enter)) {
            if (button.onClick) {
                playClickSound();
                button.onClick();
            }
        }
    }
}
```

## Visual State Coordination

The `RenderSystem` uses button states for visual feedback:

```cpp
void RenderSystem::renderButton(const ButtonComponent& button,
                                  const BoxComponent& box,
                                  const TransformComponent& transform)
{
    Color fillColor = box.fillColor;
    Color outlineColor = box.outlineColor;
    
    // Modify colors based on state
    if (button.pressed) {
        fillColor = darken(fillColor, 0.2F);
    } else if (button.hovered) {
        fillColor = lighten(fillColor, 0.15F);
        outlineColor = box.focusColor;
    }
    
    drawRect(transform, box.width, box.height, fillColor, outlineColor);
}
```

## Audio Integration

```cpp
void ButtonSystem::playHoverSound()
{
    eventBus_.publish(PlaySoundEvent{"ui_hover", 0.5F});
}

void ButtonSystem::playClickSound()
{
    eventBus_.publish(PlaySoundEvent{"ui_click", 0.8F});
}
```

## Button States

| State | Triggered By | Visual | Audio |
|-------|-------------|--------|-------|
| Normal | Default | Base colors | - |
| Hovered | Mouse enter | Brighter fill | Hover sound |
| Pressed | Mouse down | Darker fill | Click sound |
| Focused | Tab navigation | Focus outline | Navigate sound |

## Complete Usage Example

```cpp
// Create menu with buttons
void MainMenu::setup(Registry& registry)
{
    // Play button
    auto playEntity = registry.createEntity();
    registry.addComponent(playEntity, TransformComponent::create(400, 200));
    registry.addComponent(playEntity, BoxComponent::create(200, 50, 
        Color{50, 50, 70}, Color{90, 90, 120}));
    registry.addComponent(playEntity, ButtonComponent::create("Play", [this]() {
        startGame();
    }));
    registry.addComponent(playEntity, TextComponent::create("Play", 24));
    registry.addComponent(playEntity, FocusableComponent::create(0));
    registry.addComponent(playEntity, LayerComponent::create(RenderLayer::UI));
    
    // Settings button
    auto settingsEntity = registry.createEntity();
    registry.addComponent(settingsEntity, TransformComponent::create(400, 270));
    registry.addComponent(settingsEntity, BoxComponent::create(200, 50,
        Color{50, 50, 70}, Color{90, 90, 120}));
    registry.addComponent(settingsEntity, ButtonComponent::create("Settings", [this]() {
        openSettings();
    }));
    registry.addComponent(settingsEntity, TextComponent::create("Settings", 24));
    registry.addComponent(settingsEntity, FocusableComponent::create(1));
    registry.addComponent(settingsEntity, LayerComponent::create(RenderLayer::UI));
}

// In game loop
void MainMenu::processFrame(float deltaTime)
{
    // Handle input events
    Event event;
    while (window_.pollEvent(event)) {
        buttonSystem_.handleEvent(registry_, event);
        focusSystem_.handleEvent(registry_, event);
    }
    
    // Update systems
    buttonSystem_.update(registry_, deltaTime);
    
    // Render
    renderSystem_.update(registry_, deltaTime);
}
```

## Best Practices

1. **Event Order**: Process button events before other UI events
2. **Callback Safety**: Ensure callbacks don't modify button while iterating
3. **Audio Feedback**: Always provide audio for hover and click
4. **Consistent Sizing**: Use uniform button sizes for visual coherence
5. **Focus Support**: Always add FocusableComponent for keyboard users

## Performance Considerations

- **Spatial Queries**: For many buttons, consider spatial partitioning
- **Event Filtering**: Only process relevant events
- **State Caching**: Cache hover state to avoid redundant checks

## Related Components

- [ButtonComponent](../components/button-component.md) - Button state and callback
- [BoxComponent](../components/box-component.md) - Visual background
- [FocusableComponent](../components/focusable-component.md) - Keyboard navigation
- [TextComponent](../components/text-component.md) - Button label

## Related Systems

- [RenderSystem](render-system.md) - Visual rendering
- [AudioSystem](audio-system.md) - Sound playback
- [InputFieldSystem](input-field-system.md) - Related UI input handling
