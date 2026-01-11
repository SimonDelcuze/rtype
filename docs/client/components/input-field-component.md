# InputFieldComponent

## Overview

The `InputFieldComponent` is a comprehensive text input component in the R-Type client that enables users to enter text in UI forms. It supports various input types including standard text, passwords, IP addresses, and port numbers, with built-in validation capabilities. This component is essential for the game's menu system, handling player authentication, server connection, and room creation interfaces.

## Purpose and Design Philosophy

Games with online functionality require robust text input:

1. **Authentication**: Username and password entry for login/registration
2. **Server Connection**: IP address and port input for multiplayer
3. **Room Creation**: Room names and passwords
4. **Chat**: In-game messaging systems
5. **Validation**: Per-character input validation for specialized fields

The component provides the data layer for text input while the `InputFieldSystem` handles keyboard event processing and cursor management.

## Component Structure

```cpp
struct InputFieldComponent
{
    std::string value;
    std::string placeholder;
    std::size_t maxLength               = 32;
    bool focused                        = false;
    bool passwordField                  = false;
    std::function<bool(char)> validator = nullptr;

    static InputFieldComponent create(const std::string& defaultValue, std::size_t maxLen);
    static InputFieldComponent password(const std::string& defaultValue, std::size_t maxLen);
    static InputFieldComponent ipField(const std::string& defaultValue = "127.0.0.1");
    static InputFieldComponent portField(const std::string& defaultValue = "50010");
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `value` | `std::string` | `""` | The current text content of the input field. |
| `placeholder` | `std::string` | `""` | Placeholder text displayed when value is empty. |
| `maxLength` | `std::size_t` | `32` | Maximum number of characters allowed. |
| `focused` | `bool` | `false` | Whether this input field currently has input focus. |
| `passwordField` | `bool` | `false` | If true, display content as asterisks for privacy. |
| `validator` | `function<bool(char)>` | `nullptr` | Optional per-character validation function. |

## Factory Methods

### Standard Text Field

```cpp
static InputFieldComponent create(const std::string& defaultValue, std::size_t maxLen);
```

Creates a basic text input field.

```cpp
auto usernameField = InputFieldComponent::create("", 24);
usernameField.placeholder = "Enter username...";
```

### Password Field

```cpp
static InputFieldComponent password(const std::string& defaultValue, std::size_t maxLen);
```

Creates a password field that masks input.

```cpp
auto passwordField = InputFieldComponent::password("", 32);
passwordField.placeholder = "Enter password...";
```

### IP Address Field

```cpp
static InputFieldComponent ipField(const std::string& defaultValue = "127.0.0.1");
```

Creates an IP address input with numeric validation.

```cpp
auto ip = InputFieldComponent::ipField();  // Defaults to 127.0.0.1
// Validator: only digits and dots allowed
```

### Port Field

```cpp
static InputFieldComponent portField(const std::string& defaultValue = "50010");
```

Creates a port number input with numeric validation.

```cpp
auto port = InputFieldComponent::portField("50010");
// Validator: only digits allowed, max 5 characters
```

## Reset Value Support

```cpp
template <> inline void resetValue<InputFieldComponent>(InputFieldComponent& value)
{
    value = InputFieldComponent{};
}
```

## Usage Patterns

### Login Form

```cpp
void createLoginForm(Registry& registry)
{
    const float centerX = SCREEN_WIDTH / 2 - 150;
    
    // Username input
    auto username = registry.createEntity();
    registry.addComponent(username, TransformComponent::create(centerX, 200));
    registry.addComponent(username, BoxComponent::create(300, 45, 
        Color{25, 25, 35}, Color{70, 70, 90}));
    auto usernameInput = InputFieldComponent::create("", 24);
    usernameInput.placeholder = "Username";
    registry.addComponent(username, usernameInput);
    registry.addComponent(username, TextComponent::create("", 18));
    registry.addComponent(username, FocusableComponent::create(0));
    registry.addComponent(username, LayerComponent::create(RenderLayer::UI));
    
    // Password input
    auto password = registry.createEntity();
    registry.addComponent(password, TransformComponent::create(centerX, 260));
    registry.addComponent(password, BoxComponent::create(300, 45,
        Color{25, 25, 35}, Color{70, 70, 90}));
    auto passwordInput = InputFieldComponent::password("", 32);
    passwordInput.placeholder = "Password";
    registry.addComponent(password, passwordInput);
    registry.addComponent(password, TextComponent::create("", 18));
    registry.addComponent(password, FocusableComponent::create(1));
    registry.addComponent(password, LayerComponent::create(RenderLayer::UI));
}
```

### Server Connection Form

```cpp
void createConnectionForm(Registry& registry)
{
    // IP Address field
    auto ipEntity = registry.createEntity();
    registry.addComponent(ipEntity, TransformComponent::create(200, 300));
    registry.addComponent(ipEntity, BoxComponent::create(200, 40,
        Color{30, 30, 40}, Color{80, 80, 100}));
    registry.addComponent(ipEntity, InputFieldComponent::ipField("127.0.0.1"));
    registry.addComponent(ipEntity, TextComponent::create("127.0.0.1", 16));
    registry.addComponent(ipEntity, FocusableComponent::create(0));
    registry.addComponent(ipEntity, LayerComponent::create(RenderLayer::UI));
    
    // Port field
    auto portEntity = registry.createEntity();
    registry.addComponent(portEntity, TransformComponent::create(420, 300));
    registry.addComponent(portEntity, BoxComponent::create(100, 40,
        Color{30, 30, 40}, Color{80, 80, 100}));
    registry.addComponent(portEntity, InputFieldComponent::portField("50010"));
    registry.addComponent(portEntity, TextComponent::create("50010", 16));
    registry.addComponent(portEntity, FocusableComponent::create(1));
    registry.addComponent(portEntity, LayerComponent::create(RenderLayer::UI));
}
```

### Custom Validator

Creating a field with custom validation:

```cpp
// Room name: alphanumeric only, no spaces
auto roomNameInput = InputFieldComponent::create("", 16);
roomNameInput.placeholder = "Room name";
roomNameInput.validator = [](char ch) {
    return (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9') ||
           ch == '_';
};
```

## InputFieldSystem Logic

The system handles keyboard input for focused fields:

```cpp
void InputFieldSystem::handleEvent(Registry& registry, const Event& event)
{
    auto view = registry.view<InputFieldComponent, TextComponent, FocusableComponent>();
    
    for (auto entity : view) {
        auto& input = view.get<InputFieldComponent>(entity);
        auto& text = view.get<TextComponent>(entity);
        auto& focus = view.get<FocusableComponent>(entity);
        
        if (!focus.focused) continue;
        
        if (event.type == EventType::TextEntered) {
            handleTextInput(input, text, event.unicode);
        } else if (event.type == EventType::KeyPressed) {
            handleKeyPress(input, text, event.key);
        }
    }
}

void InputFieldSystem::handleTextInput(InputFieldComponent& input, 
                                         TextComponent& text,
                                         uint32_t unicode)
{
    // Ignore control characters
    if (unicode < 32 || unicode == 127) return;
    
    // Check max length
    if (input.value.length() >= input.maxLength) return;
    
    char ch = static_cast<char>(unicode);
    
    // Apply validator if present
    if (input.validator && !input.validator(ch)) return;
    
    // Add character
    input.value += ch;
    updateDisplayText(input, text);
}

void InputFieldSystem::handleKeyPress(InputFieldComponent& input,
                                        TextComponent& text,
                                        Key key)
{
    switch (key) {
        case Key::Backspace:
            if (!input.value.empty()) {
                input.value.pop_back();
                updateDisplayText(input, text);
            }
            break;
            
        case Key::Delete:
            // Handle delete key if cursor support is added
            break;
    }
}

void InputFieldSystem::updateDisplayText(InputFieldComponent& input, TextComponent& text)
{
    if (input.passwordField) {
        // Show asterisks for password
        text.content = std::string(input.value.length(), '*');
    } else {
        text.content = input.value;
    }
}
```

## Rendering Behavior

The render system displays input fields with visual feedback:

```cpp
void RenderSystem::renderInputField(const InputFieldComponent& input,
                                      const TextComponent& text,
                                      const BoxComponent& box,
                                      const TransformComponent& transform,
                                      const FocusableComponent& focus,
                                      float time)
{
    // Draw background
    Color outlineColor = focus.focused ? box.focusColor : box.outlineColor;
    drawRect(transform.x, transform.y, box.width, box.height,
             box.fillColor, outlineColor, box.outlineThickness);
    
    // Draw content or placeholder
    if (input.value.empty() && !focus.focused) {
        // Show placeholder in gray
        drawText(input.placeholder, 
                 transform.x + 10, transform.y + box.height / 2,
                 18, Color{120, 120, 130, 255});
    } else {
        // Show actual content (or asterisks for password)
        std::string display = input.passwordField ? 
            std::string(input.value.length(), '*') : input.value;
        
        drawText(display,
                 transform.x + 10, transform.y + box.height / 2,
                 18, Color{230, 230, 240, 255});
    }
    
    // Draw cursor if focused
    if (focus.focused) {
        float cursorBlink = std::fmod(time, 1.0F) < 0.5F ? 1.0F : 0.0F;
        if (cursorBlink > 0.0F) {
            float cursorX = transform.x + 10 + getTextWidth(text.content, 18);
            drawLine(cursorX, transform.y + 8,
                     cursorX, transform.y + box.height - 8,
                     Color{200, 200, 255, 255}, 2.0F);
        }
    }
}
```

## Validation Patterns

### Email Validation

```cpp
auto emailInput = InputFieldComponent::create("", 64);
emailInput.validator = [](char ch) {
    return (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9') ||
           ch == '@' || ch == '.' || ch == '_' || ch == '-';
};
```

### Numeric Only

```cpp
auto numericInput = InputFieldComponent::create("", 10);
numericInput.validator = [](char ch) {
    return ch >= '0' && ch <= '9';
};
```

### Hex Color Code

```cpp
auto hexInput = InputFieldComponent::create("", 6);
hexInput.validator = [](char ch) {
    return (ch >= '0' && ch <= '9') ||
           (ch >= 'a' && ch <= 'f') ||
           (ch >= 'A' && ch <= 'F');
};
```

## Best Practices

1. **Placeholder Text**: Always provide helpful placeholder text
2. **Max Length**: Set appropriate limits based on use case
3. **Validation Feedback**: Consider showing validation errors
4. **Tab Order**: Use FocusableComponent for form navigation
5. **Password Security**: Never log password field values
6. **Clear Button**: Consider adding a clear/reset button

## Accessibility Considerations

- **Visible Focus**: Clear visual indication of focused field
- **Keyboard Only**: Full functionality without mouse
- **Error Messages**: Clear feedback for validation failures
- **Screen Readers**: Proper labeling for accessibility tools

## Related Components

- [FocusableComponent](focusable-component.md) - Keyboard navigation
- [BoxComponent](box-component.md) - Visual background
- [TextComponent](text-component.md) - Text display

## Related Systems

- [InputFieldSystem](../systems/input-field-system.md) - Keyboard input handling
- [RenderSystem](../systems/render-system.md) - Visual rendering
