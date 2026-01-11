# CameraComponent

## Overview

The `CameraComponent` is a comprehensive camera management component in the R-Type client that controls the viewport through which players view the game world. It provides full control over camera position, zoom, rotation, and entity following capabilities, enabling dynamic camera behaviors essential for creating engaging gameplay experiences.

## Purpose and Design Philosophy

In R-Type, the camera plays a crucial role in presenting the action to players. The `CameraComponent` is designed with flexibility in mind:

1. **Viewport Control**: Defines the visible portion of the game world
2. **Smooth Following**: Can smoothly track player entities
3. **Dynamic Zoom**: Supports zoom in/out for dramatic effects
4. **Rotation Support**: Enables screen rotation for special effects
5. **Offset Control**: Allows fine-tuning camera position relative to targets

The component provides both low-level control (direct position setting) and high-level features (entity following with smoothing), making it suitable for various gameplay scenarios.

## Component Structure

```cpp
struct CameraComponent
{
    float x        = 0.0F;
    float y        = 0.0F;
    float zoom     = 1.0F;
    float offsetX  = 0.0F;
    float offsetY  = 0.0F;
    float rotation = 0.0F;
    bool active    = true;

    EntityId targetEntity  = std::numeric_limits<EntityId>::max();
    float followSmoothness = 5.0F;
    bool followEnabled     = false;

    // Factory method
    static CameraComponent create(float x, float y, float zoom = 1.0F);

    // Position control
    void setPosition(float newX, float newY);
    void move(float dx, float dy);

    // Zoom control
    void setZoom(float newZoom);
    void clampZoom(float minZoom, float maxZoom);

    // Offset control
    void setOffset(float newOffsetX, float newOffsetY);

    // Rotation control
    void setRotation(float degrees);
    void rotate(float degrees);

    // Target following
    void setTarget(EntityId entity, float smoothness = 5.0F);
    void clearTarget();

    // Reset
    void reset();
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `x` | `float` | `0.0F` | The X coordinate of the camera's center position in world space. |
| `y` | `float` | `0.0F` | The Y coordinate of the camera's center position in world space. |
| `zoom` | `float` | `1.0F` | The zoom level. Values > 1 zoom in (objects appear larger), < 1 zoom out. |
| `offsetX` | `float` | `0.0F` | Horizontal offset from the base position, useful for look-ahead effects. |
| `offsetY` | `float` | `0.0F` | Vertical offset from the base position. |
| `rotation` | `float` | `0.0F` | Camera rotation in degrees. |
| `active` | `bool` | `true` | Whether this camera is currently active. Only active cameras affect rendering. |
| `targetEntity` | `EntityId` | `MAX` | The entity to follow. Set to max value when not following any entity. |
| `followSmoothness` | `float` | `5.0F` | Smoothing factor for following. Higher values = faster following. |
| `followEnabled` | `bool` | `false` | Whether entity following is currently enabled. |

## Factory and Methods

### Factory Method

#### `create(float x, float y, float zoom = 1.0F)`

Creates a new camera at the specified position.

```cpp
// Create camera at origin with default zoom
auto camera = CameraComponent::create(0.0F, 0.0F);

// Create camera at specific position with 2x zoom
auto zoomedCamera = CameraComponent::create(500.0F, 300.0F, 2.0F);
```

### Position Control Methods

#### `setPosition(float newX, float newY)`

Sets the camera's absolute position.

```cpp
camera.setPosition(1920.0F, 540.0F);
```

#### `move(float dx, float dy)`

Moves the camera by a relative amount.

```cpp
// Move camera 10 pixels right and 5 pixels down
camera.move(10.0F, 5.0F);
```

### Zoom Control Methods

#### `setZoom(float newZoom)`

Sets the zoom level. Only accepts positive values.

```cpp
camera.setZoom(1.5F);  // 50% zoom in
camera.setZoom(0.5F);  // 50% zoom out
```

#### `clampZoom(float minZoom, float maxZoom)`

Constrains the zoom to a specified range.

```cpp
camera.clampZoom(0.5F, 3.0F);  // Limit zoom between 0.5x and 3x
```

### Offset Control

#### `setOffset(float newOffsetX, float newOffsetY)`

Sets the camera offset, useful for look-ahead effects.

```cpp
// Offset camera to show more area ahead of player
camera.setOffset(100.0F, 0.0F);
```

### Rotation Control

#### `setRotation(float degrees)`

Sets the absolute rotation angle.

```cpp
camera.setRotation(45.0F);  // 45 degree rotation
```

#### `rotate(float degrees)`

Adds to the current rotation.

```cpp
camera.rotate(10.0F);  // Add 10 degrees to current rotation
```

### Entity Following

#### `setTarget(EntityId entity, float smoothness = 5.0F)`

Configures the camera to follow an entity.

```cpp
camera.setTarget(playerEntity, 8.0F);  // Follow player with smoothness of 8
```

#### `clearTarget()`

Stops following any entity.

```cpp
camera.clearTarget();
```

### Reset

#### `reset()`

Resets all camera properties to defaults.

```cpp
camera.reset();  // Full reset to default state
```

## Usage Patterns

### Creating the Main Game Camera

```cpp
Entity createGameCamera(Registry& registry)
{
    auto entity = registry.createEntity();
    
    // Create camera centered on starting area
    auto camera = CameraComponent::create(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    camera.active = true;
    
    registry.addComponent(entity, camera);
    
    return entity;
}
```

### Player-Following Camera

Common in R-Type for tracking the player ship:

```cpp
void setupPlayerCamera(Registry& registry, Entity cameraEntity, Entity playerEntity)
{
    auto& camera = registry.getComponent<CameraComponent>(cameraEntity);
    
    // Follow player with smooth interpolation
    camera.setTarget(playerEntity, 6.0F);
    
    // Offset to show more area ahead of player movement
    camera.setOffset(150.0F, 0.0F);
}
```

### Auto-Scrolling Camera

For classic R-Type side-scrolling levels:

```cpp
void AutoScrollCameraSystem::update(Registry& registry, float deltaTime)
{
    auto view = registry.view<CameraComponent>();
    
    for (auto entity : view) {
        auto& camera = view.get<CameraComponent>(entity);
        
        if (m_autoScrollEnabled) {
            // Move camera continuously to the right
            camera.move(m_scrollSpeed * deltaTime, 0.0F);
            
            // Optionally clamp to level bounds
            if (camera.x > m_levelEndX) {
                camera.x = m_levelEndX;
            }
        }
    }
}
```

### Boss Encounter Camera

Zoom and center on boss during encounters:

```cpp
void centerOnBoss(Registry& registry, Entity cameraEntity, Entity bossEntity)
{
    auto& camera = registry.getComponent<CameraComponent>(cameraEntity);
    auto& bossTransform = registry.getComponent<TransformComponent>(bossEntity);
    
    // Smoothly center between player and boss
    camera.setTarget(bossEntity, 3.0F);  // Slow, dramatic follow
    camera.setZoom(0.8F);  // Zoom out to show both player and boss
    camera.setOffset(0.0F, 0.0F);  // Center camera on action
}
```

### Screen Shake Effect

Using camera position for screen shake:

```cpp
void applyCameraShake(CameraComponent& camera, float intensity, float duration, float elapsed)
{
    if (elapsed < duration) {
        float decay = 1.0F - (elapsed / duration);
        float shakeX = (std::rand() % 100 - 50) / 50.0F * intensity * decay;
        float shakeY = (std::rand() % 100 - 50) / 50.0F * intensity * decay;
        
        camera.setOffset(shakeX, shakeY);
    } else {
        camera.setOffset(0.0F, 0.0F);
    }
}
```

### Zoom Effect for Power-ups

```cpp
void powerUpZoomEffect(CameraComponent& camera, float elapsed, float duration)
{
    if (elapsed < duration / 2) {
        // Zoom in phase
        float t = elapsed / (duration / 2);
        camera.setZoom(1.0F + 0.2F * t);  // Zoom to 1.2x
    } else {
        // Zoom out phase
        float t = (elapsed - duration / 2) / (duration / 2);
        camera.setZoom(1.2F - 0.2F * t);  // Return to 1.0x
    }
}
```

## Integration with CameraSystem

The `CameraSystem` processes camera components each frame:

```cpp
void CameraSystem::update(Registry& registry, float deltaTime)
{
    auto cameraView = registry.view<CameraComponent>();
    
    for (auto entity : cameraView) {
        auto& camera = cameraView.get<CameraComponent>(entity);
        
        if (!camera.active) continue;
        
        // Handle entity following
        if (camera.followEnabled) {
            if (registry.entityExists(camera.targetEntity)) {
                auto& targetTransform = registry.getComponent<TransformComponent>(camera.targetEntity);
                
                // Smooth interpolation to target
                float targetX = targetTransform.x + camera.offsetX;
                float targetY = targetTransform.y + camera.offsetY;
                
                float t = 1.0F - std::exp(-camera.followSmoothness * deltaTime);
                camera.x = camera.x + (targetX - camera.x) * t;
                camera.y = camera.y + (targetY - camera.y) * t;
            } else {
                // Target no longer exists
                camera.clearTarget();
            }
        }
        
        // Apply camera transform to view
        applyViewTransform(camera);
    }
}
```

## Rendering Integration

The camera transform affects how entities are rendered:

```cpp
void RenderSystem::applyCamera(const CameraComponent& camera, Window& window)
{
    sf::View view = window.getView();
    
    // Set view center
    view.setCenter(camera.x, camera.y);
    
    // Apply zoom
    view.setSize(SCREEN_WIDTH / camera.zoom, SCREEN_HEIGHT / camera.zoom);
    
    // Apply rotation
    view.setRotation(camera.rotation);
    
    window.setView(view);
}
```

## Best Practices

1. **Single Active Camera**: Only one camera should be active at a time
2. **Smooth Transitions**: Use lerping when changing camera properties
3. **Zoom Limits**: Always clamp zoom to prevent extreme values
4. **Offset for Look-ahead**: Use offset to show more area in movement direction
5. **Clean Entity References**: Clear target when entity is destroyed
6. **Performance**: Only process active cameras

## Common Smoothness Values

| Value | Behavior | Use Case |
|-------|----------|----------|
| `2.0` | Very smooth, slow following | Cinematic sequences |
| `5.0` | Default, balanced | Normal gameplay |
| `8.0` | Responsive, snappy | Fast-paced action |
| `15.0+` | Near-instant | UI cameras, no lag |

## Related Components

- [TransformComponent](../server/components/transform-component.md) - Entity positions for following

## Related Systems

- [CameraSystem](../systems/camera-system.md) - Processes camera logic
- [RenderSystem](../systems/render-system.md) - Applies camera to rendering
