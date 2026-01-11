# InputFieldSystem

## Overview

The `InputFieldSystem` is a UI input management system in the R-Type client that handles text input for form fields. It processes keyboard events including character input, backspace, tab navigation, and mouse clicks to manage text editing in input fields throughout the game's menu system, particularly for login forms, server connection dialogs, and room creation interfaces.

## Purpose and Design Philosophy

Text input in games requires careful handling:

1. **Character Input**: Process typed characters with validation
2. **Editing Controls**: Handle backspace, delete, and cursor movement
3. **Focus Management**: Track which field receives input
4. **Tab Navigation**: Move between fields with Tab key
5. **Mouse Selection**: Click to focus specific fields

## Class Definition

```cpp
class InputFieldSystem : public ISystem
{
  public:
    InputFieldSystem(Window& window, FontManager& fonts);

    void update(Registry& registry, float deltaTime) override;
    void handleEvent(Registry& registry, const Event& event);

  private:
    void handleTextEntered(Registry& registry, char c);
    void handleBackspace(Registry& registry);
    void handleTab(Registry& registry);
    void handleClick(Registry& registry, int x, int y);

    Window& window_;
    FontManager& fonts_;
};
```

## Constructor

```cpp
InputFieldSystem(Window& window, FontManager& fonts);
```

**Parameters:**
- `window`: Reference to game window for mouse position
- `fonts`: Reference to font manager for text measurement

## Event Handling

### Main Event Handler

```cpp
void InputFieldSystem::handleEvent(Registry& registry, const Event& event)
{
    switch (event.type) {
        case EventType::TextEntered:
            handleTextEntered(registry, static_cast<char>(event.text.unicode));
            break;
            
        case EventType::KeyPressed:
            if (event.key.code == Key::Backspace) {
                handleBackspace(registry);
            } else if (event.key.code == Key::Tab) {
                handleTab(registry);
            } else if (event.key.code == Key::Return) {
                handleEnter(registry);
            }
            break;
            
        case EventType::MouseButtonPressed:
            if (event.mouseButton.button == MouseButton::Left) {
                handleClick(registry, event.mouseButton.x, event.mouseButton.y);
            }
            break;
    }
}
```

### Text Input Processing

```cpp
void InputFieldSystem::handleTextEntered(Registry& registry, char c)
{
    // Ignore control characters
    if (c < 32 || c == 127) return;
    
    auto view = registry.view<InputFieldComponent, TextComponent, FocusableComponent>();
    
    for (auto entity : view) {
        auto& input = view.get<InputFieldComponent>(entity);
        auto& text = view.get<TextComponent>(entity);
        auto& focus = view.get<FocusableComponent>(entity);
        
        // Only process for focused field
        if (!focus.focused) continue;
        
        // Check max length
        if (input.value.length() >= input.maxLength) {
            playSound("ui_error");
            return;
        }
        
        // Apply validator if present
        if (input.validator && !input.validator(c)) {
            playSound("ui_error");
            return;
        }
        
        // Add character
        input.value += c;
        updateDisplayText(input, text);
        
        playSound("ui_type");
    }
}
```

### Backspace Handling

```cpp
void InputFieldSystem::handleBackspace(Registry& registry)
{
    auto view = registry.view<InputFieldComponent, TextComponent, FocusableComponent>();
    
    for (auto entity : view) {
        auto& input = view.get<InputFieldComponent>(entity);
        auto& text = view.get<TextComponent>(entity);
        auto& focus = view.get<FocusableComponent>(entity);
        
        if (!focus.focused) continue;
        
        if (!input.value.empty()) {
            input.value.pop_back();
            updateDisplayText(input, text);
            playSound("ui_backspace");
        }
    }
}
```

### Tab Navigation

```cpp
void InputFieldSystem::handleTab(Registry& registry)
{
    auto view = registry.view<FocusableComponent>();
    
    // Collect focusable entities sorted by tab order
    std::vector<std::pair<EntityId, int>> focusables;
    
    for (auto entity : view) {
        auto& focus = view.get<FocusableComponent>(entity);
        focusables.emplace_back(entity, focus.tabOrder);
    }
    
    std::sort(focusables.begin(), focusables.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    if (focusables.empty()) return;
    
    // Find currently focused
    int currentIndex = -1;
    for (size_t i = 0; i < focusables.size(); ++i) {
        auto& focus = registry.getComponent<FocusableComponent>(focusables[i].first);
        if (focus.focused) {
            focus.focused = false;
            currentIndex = static_cast<int>(i);
            break;
        }
    }
    
    // Move to next
    int nextIndex = (currentIndex + 1) % focusables.size();
    auto& nextFocus = registry.getComponent<FocusableComponent>(focusables[nextIndex].first);
    nextFocus.focused = true;
    
    playSound("ui_navigate");
}
```

### Mouse Click Focus

```cpp
void InputFieldSystem::handleClick(Registry& registry, int x, int y)
{
    auto view = registry.view<InputFieldComponent, BoxComponent, 
                               TransformComponent, FocusableComponent>();
    
    // Clear all focus first
    for (auto entity : view) {
        auto& focus = view.get<FocusableComponent>(entity);
        focus.focused = false;
    }
    
    // Check if click is inside any input field
    for (auto entity : view) {
        auto& box = view.get<BoxComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        auto& focus = view.get<FocusableComponent>(entity);
        
        if (isPointInRect(x, y, transform, box)) {
            focus.focused = true;
            playSound("ui_click");
            break;  // Only one field can be focused
        }
    }
}

bool InputFieldSystem::isPointInRect(int x, int y, 
                                       const TransformComponent& transform,
                                       const BoxComponent& box)
{
    return x >= transform.x && x <= transform.x + box.width &&
           y >= transform.y && y <= transform.y + box.height;
}
```

## Display Text Updates

```cpp
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

## Update Loop

```cpp
void InputFieldSystem::update(Registry& registry, float deltaTime)
{
    // Update cursor blink for focused fields
    m_cursorBlinkTime += deltaTime;
    
    auto view = registry.view<InputFieldComponent, FocusableComponent>();
    
    for (auto entity : view) {
        auto& input = view.get<InputFieldComponent>(entity);
        auto& focus = view.get<FocusableComponent>(entity);
        
        // Track focused state on component
        input.focused = focus.focused;
    }
}
```

## Cursor Rendering

The render system handles cursor display:

```cpp
void RenderSystem::renderInputFieldCursor(const InputFieldComponent& input,
                                            const TextComponent& text,
                                            const TransformComponent& transform,
                                            float time)
{
    if (!input.focused) return;
    
    // Blink cursor
    float blinkPhase = std::fmod(time, 1.0F);
    if (blinkPhase > 0.5F) return;  // Invisible phase
    
    // Calculate cursor position
    std::string displayText = input.passwordField ? 
        std::string(input.value.length(), '*') : input.value;
    
    float textWidth = measureTextWidth(displayText, text.fontSize);
    float cursorX = transform.x + 8 + textWidth;  // 8px padding
    
    // Draw cursor line
    drawLine(
        cursorX, transform.y + 6,
        cursorX, transform.y + 36,
        Color{200, 200, 255, 255},
        2.0F
    );
}
```

## Placeholder Rendering

```cpp
void RenderSystem::renderInputFieldText(const InputFieldComponent& input,
                                          const TextComponent& text,
                                          const TransformComponent& transform)
{
    std::string displayText;
    Color textColor;
    
    if (input.value.empty() && !input.focused) {
        // Show placeholder
        displayText = input.placeholder;
        textColor = Color{120, 120, 140, 255};  // Dim placeholder
    } else if (input.passwordField) {
        // Show asterisks
        displayText = std::string(input.value.length(), '*');
        textColor = text.color;
    } else {
        // Show actual value
        displayText = input.value;
        textColor = text.color;
    }
    
    drawText(displayText, transform.x + 8, transform.y + 12, text.fontSize, textColor);
}
```

## Complete Usage Example

```cpp
// Create login form
void createLoginForm(Registry& registry)
{
    const float centerX = 200;
    const float fieldWidth = 280;
    const float fieldHeight = 40;
    
    // Username field
    auto username = registry.createEntity();
    registry.addComponent(username, TransformComponent::create(centerX, 180));
    registry.addComponent(username, BoxComponent::create(fieldWidth, fieldHeight,
        Color{25, 25, 35}, Color{60, 60, 80}));
    auto usernameInput = InputFieldComponent::create("", 24);
    usernameInput.placeholder = "Username";
    registry.addComponent(username, usernameInput);
    registry.addComponent(username, TextComponent::create("", 18));
    registry.addComponent(username, FocusableComponent::create(0));
    registry.addComponent(username, LayerComponent::create(RenderLayer::UI));
    
    // Password field
    auto password = registry.createEntity();
    registry.addComponent(password, TransformComponent::create(centerX, 235));
    registry.addComponent(password, BoxComponent::create(fieldWidth, fieldHeight,
        Color{25, 25, 35}, Color{60, 60, 80}));
    registry.addComponent(password, InputFieldComponent::password("", 32));
    registry.addComponent(password, TextComponent::create("", 18));
    registry.addComponent(password, FocusableComponent::create(1));
    registry.addComponent(password, LayerComponent::create(RenderLayer::UI));
    
    // Login button
    auto loginBtn = registry.createEntity();
    registry.addComponent(loginBtn, TransformComponent::create(centerX, 300));
    registry.addComponent(loginBtn, BoxComponent::create(fieldWidth, 45,
        Color{40, 80, 120}, Color{80, 120, 160}));
    registry.addComponent(loginBtn, ButtonComponent::create("Login", [this]() {
        attemptLogin();
    }));
    registry.addComponent(loginBtn, TextComponent::create("Login", 20));
    registry.addComponent(loginBtn, FocusableComponent::create(2));
    registry.addComponent(loginBtn, LayerComponent::create(RenderLayer::UI));
}

// Handle form submission
void LoginMenu::attemptLogin()
{
    auto usernameEntity = findEntityWithPlaceholder("Username");
    auto passwordEntity = findEntityWithPlaceholder("Password");
    
    auto& username = registry_.getComponent<InputFieldComponent>(usernameEntity);
    auto& password = registry_.getComponent<InputFieldComponent>(passwordEntity);
    
    if (username.value.empty() || password.value.empty()) {
        showError("Please fill in all fields");
        return;
    }
    
    authService_.login(username.value, password.value);
}
```

## Best Practices

1. **Validation**: Always use validators for specialized fields
2. **Max Length**: Set appropriate limits per field type
3. **Placeholder Text**: Provide helpful placeholder text
4. **Tab Order**: Assign logical tab orders
5. **Audio Feedback**: Confirm input with subtle sounds

## Related Components

- [InputFieldComponent](../components/input-field-component.md) - Input data/state
- [FocusableComponent](../components/focusable-component.md) - Focus management
- [BoxComponent](../components/box-component.md) - Visual background
- [TextComponent](../components/text-component.md) - Text display

## Related Systems

- [ButtonSystem](button-system.md) - Related UI handling
- [RenderSystem](render-system.md) - Text/cursor rendering
