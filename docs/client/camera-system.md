# Camera System

The Camera System manages the 2D camera view for rendering, allowing control over position, zoom, rotation, and offset. It works in conjunction with the CameraComponent component to provide flexible camera control for the game.

## Architecture

The camera system consists of two main parts:

1. **CameraComponent Component** - Stores camera properties (position, zoom, rotation, offset)
2. **CameraSystem** - Manages sf::View transformations and applies camera properties to the render window

## CameraComponent Component

The CameraComponent component stores all camera-related properties:

```cpp
struct CameraComponent {
    float x = 0.0F;         // Camera X position
    float y = 0.0F;         // Camera Y position
    float zoom = 1.0F;      // Zoom level (1.0 = normal, 2.0 = 2x zoom in)
    float offsetX = 0.0F;   // Offset X (for screen shake, etc.)
    float offsetY = 0.0F;   // Offset Y (for screen shake, etc.)
    float rotation = 0.0F;  // Rotation in degrees
    bool active = true;     // Whether this camera is active
};
```

### Factory Method

```cpp
CameraComponent camera = CameraComponent::create(100.0F, 200.0F, 2.0F);
// Creates camera at (100, 200) with 2x zoom
```

### Position Control

```cpp
camera.setPosition(400.0F, 300.0F);  // Absolute positioning
camera.move(10.0F, 5.0F);            // Relative movement
```

### Zoom Control

```cpp
camera.setZoom(2.0F);  // 2x zoom (shows half the view area)
camera.clampZoom(0.5F, 4.0F);  // Ensure zoom stays within bounds
```

**Zoom behavior:**
- `zoom = 1.0` - Normal view (shows full window size)
- `zoom = 2.0` - Zoomed in 2x (shows half the area)
- `zoom = 0.5` - Zoomed out 2x (shows double the area)

### Offset (Screen Shake)

```cpp
camera.setOffset(10.0F, 5.0F);  // Offset view center by (10, 5)
```

The offset is added to the camera position when calculating the final view center. This is useful for:
- Screen shake effects
- Camera wobble
- Temporary view adjustments

### Rotation

```cpp
camera.setRotation(45.0F);  // Set rotation to 45 degrees
camera.rotate(10.0F);       // Rotate by additional 10 degrees
```

### Reset

```cpp
camera.reset();  // Reset to default values (0, 0, zoom 1.0)
```

## CameraSystem

The CameraSystem applies CameraComponent properties to the SFML render window's view.

### Initialization

```cpp
sf::RenderWindow window(sf::VideoMode({800u, 600u}), "Game");
CameraSystem cameraSystem(window);
```

### Update Loop

```cpp
void gameLoop() {
    // Update camera based on active CameraComponent component
    cameraSystem.update(registry);

    // Render using the updated view
    // (view is automatically set on the window)
}
```

### Active Camera Selection

The system automatically finds the first active CameraComponent component:

```cpp
EntityId camera1 = registry.createEntity();
EntityId camera2 = registry.createEntity();

auto& cam1 = registry.emplace<CameraComponent>(camera1);
auto& cam2 = registry.emplace<CameraComponent>(camera2);

cam1.active = true;   // This camera will be used
cam2.active = false;  // This camera is ignored
```

To switch cameras, simply toggle the `active` flag:

```cpp
cam1.active = false;
cam2.active = true;
```

### World Bounds Clamping

Prevent the camera from moving outside defined world boundaries:

```cpp
// Define world bounds: left, top, width, height
cameraSystem.setWorldBounds(0.0F, 0.0F, 1920.0F, 1080.0F);

// Enable clamping
cameraSystem.setWorldBoundsEnabled(true);

// Camera will now be clamped to stay within bounds
```

**Clamping behavior:**
- Takes zoom level into account (prevents showing area outside bounds)
- If world is smaller than view, camera centers on world
- Can be enabled/disabled at runtime

### Accessing the View

```cpp
const sf::View& view = cameraSystem.getView();
sf::Vector2f center = view.getCenter();
sf::Vector2f size = view.getSize();
```

### Getting Active Camera

```cpp
EntityId activeCameraId = cameraSystem.getActiveCamera();
if (activeCameraId != 0) {
    auto& camera = registry.get<CameraComponent>(activeCameraId);
    // Modify camera properties...
}
```

## Usage Examples

### Basic Camera Follow

```cpp
// Update camera to follow player
EntityId player = getPlayerEntity();
EntityId camera = getCameraEntity();

const auto& playerTransform = registry.get<TransformComponent>(player);
auto& cam = registry.get<CameraComponent>(camera);

cam.setPosition(playerTransform.x, playerTransform.y);
```

### Smooth Camera Follow

```cpp
// Smooth camera interpolation
const auto& playerPos = registry.get<TransformComponent>(player);
auto& cam = registry.get<CameraComponent>(camera);

float lerpFactor = 0.1F;
float dx = (playerPos.x - cam.x) * lerpFactor;
float dy = (playerPos.y - cam.y) * lerpFactor;

cam.move(dx, dy);
```

### Screen Shake Effect

```cpp
// Simple screen shake on impact
void applyScreenShake(CameraComponent& camera, float intensity) {
    float shakeX = (rand() / (float)RAND_MAX - 0.5F) * intensity * 2.0F;
    float shakeY = (rand() / (float)RAND_MAX - 0.5F) * intensity * 2.0F;

    camera.setOffset(shakeX, shakeY);
}

// Gradually reduce shake over time
void updateScreenShake(CameraComponent& camera, float deltaTime) {
    camera.offsetX *= (1.0F - deltaTime * 5.0F);  // Decay
    camera.offsetY *= (1.0F - deltaTime * 5.0F);

    if (std::abs(camera.offsetX) < 0.1F) camera.offsetX = 0.0F;
    if (std::abs(camera.offsetY) < 0.1F) camera.offsetY = 0.0F;
}
```

### Zoom Control

```cpp
// Zoom in/out based on input
void handleZoomInput(CameraComponent& camera) {
    if (zoomInPressed) {
        camera.setZoom(camera.zoom * 1.1F);
    }
    if (zoomOutPressed) {
        camera.setZoom(camera.zoom / 1.1F);
    }

    // Keep zoom in reasonable range
    camera.clampZoom(0.5F, 4.0F);
}
```

### Multiple Camera Setup

```cpp
// Main game camera
EntityId mainCamera = registry.createEntity();
auto& mainCam = registry.emplace<CameraComponent>(mainCamera);
mainCam.active = true;

// Minimap camera (inactive by default)
EntityId minimapCamera = registry.createEntity();
auto& miniCam = registry.emplace<CameraComponent>(minimapCamera);
miniCam.active = false;
miniCam.setZoom(0.25F);  // Zoomed out for overview

// Switch to minimap
mainCam.active = false;
miniCam.active = true;
```

## Integration with RenderSystem

The CameraSystem should be updated before the RenderSystem:

```cpp
// In game loop
cameraSystem.update(registry);  // Apply camera transformations
renderSystem.update(registry);   // Render sprites using current view
```

The RenderSystem automatically uses the window's current view, which was set by the CameraSystem.

## Best Practices

1. **Single Active Camera** - Only one camera should be active at a time
2. **Update Before Render** - Always update CameraSystem before RenderSystem
3. **Component Reference Safety** - Don't hold references to CameraComponent across system updates (registry may reallocate)
4. **Zoom Validation** - Use `clampZoom()` to prevent extreme zoom values
5. **World Bounds** - Enable world bounds clamping for bounded levels
6. **Screen Shake Decay** - Always decay screen shake offset back to zero
7. **Smooth Following** - Use interpolation for smooth camera movement

## Common Patterns

### Camera Controller System

Create a dedicated system for camera control logic:

```cpp
class CameraControllerSystem : public ISystem {
public:
    void update(Registry& registry, float deltaTime) override {
        // Find player
        EntityId player = findPlayer(registry);
        if (player == 0) return;

        // Find active camera
        EntityId cameraId = findActiveCamera(registry);
        if (cameraId == 0) return;

        const auto& playerTransform = registry.get<TransformComponent>(player);
        auto& camera = registry.get<CameraComponent>(cameraId);

        // Smooth follow
        float lerpSpeed = 5.0F;
        float dx = (playerTransform.x - camera.x) * lerpSpeed * deltaTime;
        float dy = (playerTransform.y - camera.y) * lerpSpeed * deltaTime;
        camera.move(dx, dy);

        // Update screen shake
        updateScreenShake(camera, deltaTime);
    }
};
```

### Camera Zones

Define different camera behaviors for different areas:

```cpp
struct CameraZone {
    float left, top, width, height;
    float zoom;
    bool constrainToBounds;
};

// When player enters zone, adjust camera settings
void onPlayerEnterZone(CameraComponent& camera, const CameraZone& zone) {
    camera.setZoom(zone.zoom);

    if (zone.constrainToBounds) {
        cameraSystem.setWorldBounds(
            zone.left, zone.top,
            zone.width, zone.height
        );
        cameraSystem.setWorldBoundsEnabled(true);
    }
}
```

## Implementation Notes

- The system stores the base window size for zoom calculations
- Component references may be invalidated by world bounds clamping
- View is automatically set on the window after each update
- Rotation uses degrees (converted to sf::Angle internally)
- No camera active means default view is used
- Dead entities with CameraComponent are automatically skipped
