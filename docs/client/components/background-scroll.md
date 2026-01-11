# BackgroundScrollComponent

## Overview

The `BackgroundScrollComponent` is a specialized component within the R-Type client architecture that handles the continuous scrolling behavior of background elements. This component is fundamental to creating the classic side-scrolling shooter experience, providing smooth parallax effects and seamless background looping that gives players the sensation of forward movement through space.

## Purpose and Design Philosophy

In classic shoot-em-up games like R-Type, the background scrolling effect is crucial for gameplay immersion. The `BackgroundScrollComponent` implements a data-driven approach to background animation, storing all the necessary parameters for the `BackgroundScrollSystem` to compute and apply smooth scrolling transformations each frame.

The component follows the Entity Component System (ECS) paradigm, serving purely as a data container without any behavior logic. This separation of concerns allows for efficient batch processing of all background entities and enables easy customization of scrolling parameters per entity.

## Component Structure

```cpp
struct BackgroundScrollComponent
{
    float speedX       = 0.0F;
    float speedY       = 0.0F;
    float resetOffsetX = 0.0F;
    float resetOffsetY = 0.0F;

    static BackgroundScrollComponent create(float sx, float sy, float resetX, float resetY = 0.0F);
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `speedX` | `float` | `0.0F` | The horizontal scrolling speed in pixels per second. Positive values scroll the background to the left (creating the illusion of rightward movement), while negative values scroll right. |
| `speedY` | `float` | `0.0F` | The vertical scrolling speed in pixels per second. Positive values scroll downward, negative values scroll upward. |
| `resetOffsetX` | `float` | `0.0F` | The horizontal position threshold at which the background should wrap around. When the background's X position exceeds this value, it resets to create seamless looping. |
| `resetOffsetY` | `float` | `0.0F` | The vertical position threshold for wrapping. Functions similarly to `resetOffsetX` but for vertical scrolling. |

## Factory Method

### `create(float sx, float sy, float resetX, float resetY = 0.0F)`

Creates a new `BackgroundScrollComponent` with the specified scrolling parameters.

**Parameters:**
- `sx`: Horizontal scroll speed in pixels per second
- `sy`: Vertical scroll speed in pixels per second
- `resetX`: Horizontal reset/wrap position
- `resetY`: Vertical reset/wrap position (optional, defaults to 0.0F)

**Returns:** A fully configured `BackgroundScrollComponent` instance.

**Example:**
```cpp
// Create a horizontally scrolling background at 100 pixels/second
// that wraps after 1920 pixels (full screen width)
auto background = BackgroundScrollComponent::create(100.0F, 0.0F, 1920.0F);

// Create a parallax layer with slower speed
auto parallaxLayer = BackgroundScrollComponent::create(50.0F, 0.0F, 1920.0F);
```

## Usage Patterns

### Basic Horizontal Scrolling

The most common use case is creating a horizontally scrolling space background:

```cpp
// Create background entity
auto entity = registry.createEntity();

// Add sprite component for the background image
registry.addComponent(entity, SpriteComponent::create("space_bg", 0, 0));

// Add scrolling behavior
registry.addComponent(entity, BackgroundScrollComponent::create(
    150.0F,    // Speed: 150 pixels per second
    0.0F,      // No vertical scrolling
    1920.0F    // Reset after full screen width
));

// Place in background layer
registry.addComponent(entity, LayerComponent::create(RenderLayer::Background));
```

### Parallax Scrolling Effect

To create depth through parallax, use multiple background layers with different speeds:

```cpp
// Far background (slowest - appears furthest)
auto farBg = registry.createEntity();
registry.addComponent(farBg, SpriteComponent::create("stars_far", 0, 0));
registry.addComponent(farBg, BackgroundScrollComponent::create(30.0F, 0.0F, 1920.0F));
registry.addComponent(farBg, LayerComponent::create(RenderLayer::Background - 20));

// Mid background (medium speed)
auto midBg = registry.createEntity();
registry.addComponent(midBg, SpriteComponent::create("nebula", 0, 0));
registry.addComponent(midBg, BackgroundScrollComponent::create(75.0F, 0.0F, 1920.0F));
registry.addComponent(midBg, LayerComponent::create(RenderLayer::Background - 10));

// Near background (fastest - appears closest)
auto nearBg = registry.createEntity();
registry.addComponent(nearBg, SpriteComponent::create("asteroids", 0, 0));
registry.addComponent(nearBg, BackgroundScrollComponent::create(150.0F, 0.0F, 1920.0F));
registry.addComponent(nearBg, LayerComponent::create(RenderLayer::Background));
```

### Vertical Scrolling

For vertical-scrolling sections or special effects:

```cpp
auto verticalBg = BackgroundScrollComponent::create(
    0.0F,      // No horizontal scrolling
    80.0F,     // Vertical speed
    0.0F,      // No horizontal reset
    1080.0F    // Reset after full screen height
);
```

### Combined Diagonal Scrolling

For diagonal movement effects:

```cpp
auto diagonalBg = BackgroundScrollComponent::create(
    100.0F,    // Horizontal speed
    25.0F,     // Slight vertical drift
    1920.0F,   // Horizontal reset
    1080.0F    // Vertical reset
);
```

## Integration with BackgroundScrollSystem

The `BackgroundScrollComponent` is processed by the `BackgroundScrollSystem`, which performs the following operations each frame:

1. **Position Update**: Calculates the new position based on speed and delta time
2. **Boundary Check**: Determines if the position has exceeded the reset threshold
3. **Wrap Reset**: Applies modular arithmetic to create seamless looping
4. **Transform Application**: Updates the entity's `TransformComponent` with the new position

The system operates on all entities that have both a `BackgroundScrollComponent` and a `TransformComponent`, making it efficient to manage multiple background layers simultaneously.

## Performance Considerations

- **Batch Processing**: All background entities are processed together, minimizing cache misses
- **No Allocations**: The component uses only primitive types, avoiding runtime memory allocation
- **Predictable Memory**: Fixed-size structure enables efficient storage in component arrays
- **Seamless Wrapping**: Reset logic prevents texture coordinate issues at boundaries

## Best Practices

1. **Match Reset to Texture Size**: Set `resetOffsetX`/`resetOffsetY` to match your background texture dimensions for seamless tiling
2. **Use Consistent Layer Ordering**: Combine with `LayerComponent` to ensure proper draw order
3. **Parallax Ratio Guidelines**: For convincing parallax, use speed ratios like 1:2:4 for near:mid:far layers
4. **Performance**: Limit the number of scrolling background layers to 3-5 for optimal performance

## Related Components

- [SpriteComponent](sprite-component.md) - Provides the visual representation
- [LayerComponent](layer-component.md) - Controls draw order for parallax effects
- [TransformComponent](../server/components/transform-component.md) - Stores the position data

## Related Systems

- [BackgroundScrollSystem](../systems/background-scroll.md) - The system that processes this component
