# FocusableComponent

## Overview

The `FocusableComponent` is a UI navigation component in the R-Type client that enables keyboard and gamepad navigation through interactive UI elements. It implements a tab-order system that allows players to navigate menus with arrow keys, Tab, and gamepad inputs, providing an accessible alternative to mouse interaction.

## Purpose and Design Philosophy

Modern games must support multiple input methods:

1. **Keyboard Navigation**: Tab and arrow keys for menu navigation
2. **Gamepad Support**: D-pad and stick navigation for controller users
3. **Accessibility**: Essential for players who cannot use a mouse
4. **Consistent Experience**: Same navigation logic across all menus

The component stores focus state and tab order, while the focus navigation system handles input processing and focus transitions.

## Component Structure

```cpp
struct FocusableComponent
{
    int tabOrder = 0;
    bool focused = false;

    static FocusableComponent create(int order);
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `tabOrder` | `int` | `0` | The navigation order for this element. Lower values are focused first. Elements with the same tab order are navigated in entity creation order. |
| `focused` | `bool` | `false` | Whether this element currently has keyboard/gamepad focus. |

## Factory Method

### `create(int order)`

Creates a new `FocusableComponent` with the specified tab order.

**Parameters:**
- `order`: The tab order position (lower = earlier in navigation sequence)

**Returns:** A configured `FocusableComponent` instance.

**Example:**
```cpp
// First focusable element
auto first = FocusableComponent::create(0);

// Second in tab order
auto second = FocusableComponent::create(1);

// Will be focused third
auto third = FocusableComponent::create(2);
```

## Reset Value Support

The component implements ECS reset functionality:

```cpp
template <> inline void resetValue<FocusableComponent>(FocusableComponent& value)
{
    value = FocusableComponent{};
}
```

## Usage Patterns

### Basic Menu Setup

Creating a menu with navigable buttons:

```cpp
void createMainMenu(Registry& registry)
{
    // Title (not focusable)
    createLabel(registry, "R-TYPE", SCREEN_WIDTH / 2, 100);
    
    // Play button - first in tab order
    createButton(registry, "Play", SCREEN_WIDTH / 2, 250, 
                 []() { startGame(); })
        .withFocus(0);
    
    // Settings button - second
    createButton(registry, "Settings", SCREEN_WIDTH / 2, 320,
                 []() { openSettings(); })
        .withFocus(1);
    
    // Quit button - third
    createButton(registry, "Quit", SCREEN_WIDTH / 2, 390,
                 []() { quitGame(); })
        .withFocus(2);
}

Entity createButton(Registry& registry, const std::string& label, 
                    float x, float y, std::function<void()> onClick)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    registry.addComponent(entity, BoxComponent::create(200, 50, 
        Color{50, 50, 60}, Color{80, 80, 100}));
    registry.addComponent(entity, ButtonComponent::create(label, onClick));
    registry.addComponent(entity, TextComponent::create(label, 20));
    registry.addComponent(entity, LayerComponent::create(RenderLayer::UI));
    
    return entity;
}

// Extension method for fluent API
Entity& Entity::withFocus(int order)
{
    registry.addComponent(*this, FocusableComponent::create(order));
    return *this;
}
```

### Form with Input Fields

Navigating through a login form:

```cpp
void createLoginForm(Registry& registry)
{
    // Username field - first
    auto username = createInputField(registry, "Username", 400, 200);
    registry.addComponent(username, FocusableComponent::create(0));
    
    // Password field - second
    auto password = createInputField(registry, "Password", 400, 270);
    registry.addComponent(password, FocusableComponent::create(1));
    
    // Login button - third
    auto loginBtn = createButton(registry, "Login", 400, 350, []() { submitLogin(); });
    registry.addComponent(loginBtn, FocusableComponent::create(2));
    
    // Cancel button - fourth
    auto cancelBtn = createButton(registry, "Cancel", 400, 410, []() { goBack(); });
    registry.addComponent(cancelBtn, FocusableComponent::create(3));
}
```

### Grid Navigation

For settings or inventory grids:

```cpp
void createSettingsGrid(Registry& registry)
{
    // Create 3x3 grid of options
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int tabOrder = row * 3 + col;  // 0, 1, 2, 3, 4, 5, 6, 7, 8
            
            auto option = createSettingToggle(registry, 
                200 + col * 150, 
                150 + row * 80,
                settingNames[tabOrder]);
            
            registry.addComponent(option, FocusableComponent::create(tabOrder));
        }
    }
}
```

## Focus Navigation System

The system that processes focus navigation:

```cpp
void FocusNavigationSystem::update(Registry& registry, const Event& event)
{
    auto view = registry.view<FocusableComponent>();
    std::vector<Entity> focusables;
    
    // Collect and sort by tab order
    for (auto entity : view) {
        focusables.push_back(entity);
    }
    
    std::sort(focusables.begin(), focusables.end(), [&](Entity a, Entity b) {
        return registry.getComponent<FocusableComponent>(a).tabOrder <
               registry.getComponent<FocusableComponent>(b).tabOrder;
    });
    
    if (focusables.empty()) return;
    
    // Find currently focused element
    auto focusedIt = std::find_if(focusables.begin(), focusables.end(), [&](Entity e) {
        return registry.getComponent<FocusableComponent>(e).focused;
    });
    
    // Handle navigation input
    if (event.type == EventType::KeyPressed) {
        switch (event.key) {
            case Key::Tab:
            case Key::Down:
            case Key::Right:
                moveFocusForward(registry, focusables, focusedIt);
                break;
                
            case Key::Up:
            case Key::Left:
                moveFocusBackward(registry, focusables, focusedIt);
                break;
                
            case Key::Enter:
            case Key::Space:
                activateFocused(registry, focusedIt);
                break;
        }
    }
}

void FocusNavigationSystem::moveFocusForward(Registry& registry,
                                               std::vector<Entity>& focusables,
                                               std::vector<Entity>::iterator& current)
{
    // Clear current focus
    if (current != focusables.end()) {
        registry.getComponent<FocusableComponent>(*current).focused = false;
    }
    
    // Move to next
    if (current == focusables.end() || current + 1 == focusables.end()) {
        current = focusables.begin();  // Wrap around
    } else {
        ++current;
    }
    
    // Set new focus
    registry.getComponent<FocusableComponent>(*current).focused = true;
    
    // Play navigation sound
    playSound("ui_navigate");
}
```

## Visual Feedback Integration

Focus state affects visual rendering:

```cpp
void RenderSystem::renderFocusable(Entity entity, const BoxComponent& box,
                                    const TransformComponent& transform,
                                    const FocusableComponent& focus)
{
    Color outlineColor = focus.focused ? box.focusColor : box.outlineColor;
    
    // Draw box with appropriate outline
    drawRect(transform.x, transform.y, box.width, box.height,
             box.fillColor, outlineColor, box.outlineThickness);
    
    // Add focus indicator if focused
    if (focus.focused) {
        // Draw animated focus ring
        float pulse = 0.5F + 0.5F * std::sin(m_time * 4.0F);
        Color glowColor = box.focusColor;
        glowColor.a = static_cast<uint8_t>(100 * pulse);
        
        drawRect(transform.x - 3, transform.y - 3,
                 box.width + 6, box.height + 6,
                 Color{0, 0, 0, 0}, glowColor, 2.0F);
    }
}
```

## Gamepad Integration

Mapping gamepad input to focus navigation:

```cpp
void GamepadNavigationSystem::update(Registry& registry, const GamepadState& gamepad)
{
    // D-pad navigation
    if (gamepad.dpadDown.justPressed || gamepad.leftStick.y > 0.5F) {
        moveFocusDown(registry);
    } else if (gamepad.dpadUp.justPressed || gamepad.leftStick.y < -0.5F) {
        moveFocusUp(registry);
    } else if (gamepad.dpadLeft.justPressed || gamepad.leftStick.x < -0.5F) {
        moveFocusLeft(registry);
    } else if (gamepad.dpadRight.justPressed || gamepad.leftStick.x > 0.5F) {
        moveFocusRight(registry);
    }
    
    // A button = confirm
    if (gamepad.buttonA.justPressed) {
        activateFocused(registry);
    }
    
    // B button = back/cancel
    if (gamepad.buttonB.justPressed) {
        goBack();
    }
}
```

## Initial Focus

Setting initial focus when menu opens:

```cpp
void Menu::onOpen(Registry& registry)
{
    auto view = registry.view<FocusableComponent>();
    
    // Find element with lowest tab order
    Entity firstFocusable = entt::null;
    int lowestOrder = std::numeric_limits<int>::max();
    
    for (auto entity : view) {
        auto& focus = view.get<FocusableComponent>(entity);
        focus.focused = false;  // Clear all focus first
        
        if (focus.tabOrder < lowestOrder) {
            lowestOrder = focus.tabOrder;
            firstFocusable = entity;
        }
    }
    
    // Set focus to first element
    if (firstFocusable != entt::null) {
        registry.getComponent<FocusableComponent>(firstFocusable).focused = true;
    }
}
```

## Group Navigation

For complex layouts with groups:

```cpp
// Tab order encoding: (group * 100) + position
// Group 0 (main menu): 0-99
// Group 1 (sub menu): 100-199
// etc.

void createComplexMenu(Registry& registry)
{
    // Main menu buttons (group 0)
    createButton(registry, "Play").withFocus(0);
    createButton(registry, "Options").withFocus(1);
    createButton(registry, "Exit").withFocus(2);
    
    // Options sub-menu (group 1)
    createSlider(registry, "Volume").withFocus(100);
    createToggle(registry, "Fullscreen").withFocus(101);
    createButton(registry, "Back").withFocus(102);
}
```

## Best Practices

1. **Sequential Order**: Use sequential tab orders for logical navigation
2. **Clear Visual Feedback**: Make focused state clearly visible
3. **Audio Feedback**: Play sounds on navigation for confirmation
4. **Wrap Around**: Allow navigation to wrap from last to first
5. **Initial Focus**: Always set initial focus when menu opens
6. **Consistent Controls**: Use same controls across all menus

## Accessibility Considerations

- **High Contrast Focus**: Ensure focus indicator is visible against all backgrounds
- **Audio Cues**: Provide audio feedback for focus changes
- **No Time Limits**: Don't auto-advance focus or require timed input
- **Clear Navigation**: Logical, predictable focus order

## Related Components

- [ButtonComponent](button-component.md) - Interactive buttons
- [BoxComponent](box-component.md) - Visual styling with focus color
- [InputFieldComponent](input-field-component.md) - Text input fields

## Related Systems

- [InputFieldSystem](../systems/input-field-system.md) - Handles focused input fields
- [ButtonSystem](../systems/button-system.md) - Button activation
