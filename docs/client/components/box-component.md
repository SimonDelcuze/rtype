# BoxComponent

## Overview

The `BoxComponent` is a fundamental UI component in the R-Type client that provides rectangular visual elements with customizable styling. It serves as a building block for creating UI panels, containers, input field backgrounds, button backgrounds, and various other rectangular interface elements throughout the game's menu system.

## Purpose and Design Philosophy

Modern game UIs require flexible, reusable components that can be styled consistently across the application. The `BoxComponent` follows this principle by providing:

1. **Visual Consistency**: Standardized rectangular elements with consistent styling options
2. **Focus Indication**: Built-in support for focus states, essential for keyboard/gamepad navigation
3. **Theming Support**: Color properties that can be configured to match the game's visual identity
4. **ECS Integration**: Full integration with the Entity Component System architecture

The component is purely data-driven, containing no rendering logic. Visual output is handled by the `RenderSystem`, which processes `BoxComponent` data to draw rectangles with the specified properties.

## Component Structure

```cpp
struct BoxComponent
{
    float width            = 100.0F;
    float height           = 50.0F;
    Color fillColor        = Color{50, 50, 50};
    Color outlineColor     = Color{100, 100, 100};
    Color focusColor       = Color{100, 200, 255};
    float outlineThickness = 2.0F;

    static BoxComponent create(float w, float h, Color fill, Color outline);
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `width` | `float` | `100.0F` | The width of the box in pixels. |
| `height` | `float` | `50.0F` | The height of the box in pixels. |
| `fillColor` | `Color` | `{50, 50, 50}` | The interior fill color of the box (dark gray by default). |
| `outlineColor` | `Color` | `{100, 100, 100}` | The color of the box's border outline (medium gray by default). |
| `focusColor` | `Color` | `{100, 200, 255}` | The outline color when the box is in a focused state (light blue by default). |
| `outlineThickness` | `float` | `2.0F` | The thickness of the border outline in pixels. |

### Color Structure

The `Color` type used by `BoxComponent` is defined in the graphics abstraction layer:

```cpp
struct Color
{
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
};
```

## Factory Method

### `create(float w, float h, Color fill, Color outline)`

Creates a new `BoxComponent` with the specified dimensions and colors.

**Parameters:**
- `w`: Width in pixels
- `h`: Height in pixels
- `fill`: Fill color for the interior
- `outline`: Color for the border outline

**Returns:** A configured `BoxComponent` instance.

**Example:**
```cpp
// Create a dark panel with light gray border
auto panel = BoxComponent::create(400.0F, 300.0F, Color{30, 30, 40}, Color{80, 80, 100});

// Create a button background
auto buttonBg = BoxComponent::create(200.0F, 50.0F, Color{60, 60, 80}, Color{120, 120, 150});
```

## Reset Value Support

The `BoxComponent` implements the `resetValue` template specialization for proper component pooling:

```cpp
template <> inline void resetValue<BoxComponent>(BoxComponent& value)
{
    value = BoxComponent{};
}
```

This ensures that when an entity is destroyed and its components are recycled, the `BoxComponent` returns to its default state.

## Usage Patterns

### Creating a UI Panel

```cpp
Entity createUIPanel(Registry& registry, float x, float y, float width, float height)
{
    auto entity = registry.createEntity();
    
    // Position the panel
    registry.addComponent(entity, TransformComponent::create(x, y));
    
    // Add the box visual
    registry.addComponent(entity, BoxComponent::create(
        width, height,
        Color{40, 40, 50, 230},    // Semi-transparent dark fill
        Color{100, 100, 120, 255}  // Solid lighter border
    ));
    
    // Ensure it renders on UI layer
    registry.addComponent(entity, LayerComponent::create(RenderLayer::UI));
    
    return entity;
}
```

### Input Field Background

Used in conjunction with `InputFieldComponent` and `FocusableComponent`:

```cpp
Entity createInputField(Registry& registry, float x, float y, const std::string& placeholder)
{
    auto entity = registry.createEntity();
    
    // Position
    registry.addComponent(entity, TransformComponent::create(x, y));
    
    // Visual background
    auto box = BoxComponent::create(
        300.0F, 40.0F,
        Color{20, 20, 30},     // Dark input background
        Color{70, 70, 90}      // Subtle border
    );
    box.focusColor = Color{100, 180, 255};  // Blue glow when focused
    registry.addComponent(entity, box);
    
    // Input field logic
    auto input = InputFieldComponent::create("", 64);
    input.placeholder = placeholder;
    registry.addComponent(entity, input);
    
    // Enable focus navigation
    registry.addComponent(entity, FocusableComponent::create(0));
    
    // Text display
    registry.addComponent(entity, TextComponent::create("", 18));
    
    // UI layer
    registry.addComponent(entity, LayerComponent::create(RenderLayer::UI));
    
    return entity;
}
```

### Button with Hover Effect

Combined with `ButtonComponent` for interactive buttons:

```cpp
Entity createButton(Registry& registry, float x, float y, const std::string& label)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    
    // Button background
    auto box = BoxComponent::create(
        220.0F, 50.0F,
        Color{50, 50, 70},      // Normal state
        Color{80, 80, 110}      // Border
    );
    box.focusColor = Color{120, 200, 255};  // Highlight when focused
    registry.addComponent(entity, box);
    
    // Button behavior
    registry.addComponent(entity, ButtonComponent::create(label, []{
        // Click handler
    }));
    
    // Text label
    registry.addComponent(entity, TextComponent::create(label, 20));
    
    // Focus support
    registry.addComponent(entity, FocusableComponent::create(0));
    
    registry.addComponent(entity, LayerComponent::create(RenderLayer::UI));
    
    return entity;
}
```

### Modal Dialog Background

```cpp
Entity createModalBackground(Registry& registry)
{
    auto entity = registry.createEntity();
    
    // Center of screen
    registry.addComponent(entity, TransformComponent::create(
        SCREEN_WIDTH / 2 - 250,
        SCREEN_HEIGHT / 2 - 150
    ));
    
    // Large modal panel
    auto box = BoxComponent::create(
        500.0F, 300.0F,
        Color{35, 35, 45, 240},     // Dark, slightly transparent
        Color{100, 100, 130, 255}   // Solid border
    );
    box.outlineThickness = 3.0F;    // Thicker border for emphasis
    registry.addComponent(entity, box);
    
    registry.addComponent(entity, LayerComponent::create(RenderLayer::UI + 10));
    
    return entity;
}
```

## Rendering Behavior

The `RenderSystem` processes `BoxComponent` entities as follows:

1. **Retrieve Transform**: Get the entity's position from `TransformComponent`
2. **Check Focus State**: If entity has `FocusableComponent`, check if focused
3. **Determine Outline Color**: Use `focusColor` if focused, otherwise `outlineColor`
4. **Draw Fill Rectangle**: Render the interior with `fillColor`
5. **Draw Outline**: Render the border with appropriate color and thickness

```cpp
void RenderSystem::renderBox(const BoxComponent& box, const TransformComponent& transform, bool isFocused)
{
    // Create rectangle shape
    sf::RectangleShape rect(sf::Vector2f(box.width, box.height));
    rect.setPosition(transform.x, transform.y);
    
    // Set fill color
    rect.setFillColor(sf::Color(box.fillColor.r, box.fillColor.g, box.fillColor.b, box.fillColor.a));
    
    // Set outline
    Color outlineToUse = isFocused ? box.focusColor : box.outlineColor;
    rect.setOutlineColor(sf::Color(outlineToUse.r, outlineToUse.g, outlineToUse.b, outlineToUse.a));
    rect.setOutlineThickness(box.outlineThickness);
    
    window.draw(rect);
}
```

## Styling Guidelines

### Recommended Color Schemes

**Dark Theme (Default):**
```cpp
Color fillDark{30, 30, 40};
Color fillMedium{50, 50, 65};
Color outlineSubtle{70, 70, 90};
Color outlineNormal{100, 100, 130};
Color focusBlue{100, 180, 255};
```

**Accent Colors:**
```cpp
Color successGreen{80, 200, 120};
Color warningYellow{220, 180, 60};
Color errorRed{200, 80, 80};
Color infoBlue{80, 150, 220};
```

### Transparency Guidelines

- **Solid Panels**: Alpha = 255
- **Overlay Panels**: Alpha = 230-245
- **Background Dimming**: Alpha = 180-200
- **Subtle Overlays**: Alpha = 100-150

## Best Practices

1. **Consistent Sizing**: Use standardized sizes for similar UI elements
2. **Focus Visibility**: Ensure `focusColor` has sufficient contrast for accessibility
3. **Layer Management**: Always pair with `LayerComponent` for correct rendering order
4. **Outline Thickness**: Keep between 1-4 pixels for clean rendering
5. **Alpha Blending**: Use transparency sparingly for performance

## Performance Considerations

- **Batched Rendering**: Multiple boxes can be batched if using same texture
- **Minimal Overdraw**: Opaque boxes don't require alpha blending
- **Simple Geometry**: Rectangles are the most efficient shapes to render

## Related Components

- [FocusableComponent](focusable-component.md) - Enables focus navigation
- [ButtonComponent](button-component.md) - Interactive button behavior
- [InputFieldComponent](input-field-component.md) - Text input functionality
- [TextComponent](text-component.md) - Text labels
- [LayerComponent](layer-component.md) - Render ordering

## Related Systems

- [RenderSystem](../systems/render-system.md) - Handles box rendering
- [ButtonSystem](../systems/button-system.md) - Manages hover/pressed states
- [InputFieldSystem](../systems/input-field-system.md) - Coordinates input field visuals
