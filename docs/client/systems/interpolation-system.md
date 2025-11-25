# InterpolationSystem

The InterpolationSystem smooths entity movement between server updates using client-side interpolation and extrapolation.

## Overview

In multiplayer games, server updates arrive at discrete intervals (e.g., every 100ms). Without interpolation, entities would "teleport" between positions, creating jerky movement. The InterpolationSystem solves this by:

- **Interpolating**: Smoothly transitioning between the previous and target positions
- **Extrapolating**: Predicting future positions when updates are delayed

## Components

### InterpolationComponent

```cpp
#include "components/InterpolationComponent.hpp"

enum class InterpolationMode : std::uint8_t
{
    None,          
    Linear,        
    Extrapolate    
};

struct InterpolationComponent
{
    // Previous and target positions from server
    float previousX = 0.0F;
    float previousY = 0.0F;
    float targetX   = 0.0F;
    float targetY   = 0.0F;

    float elapsedTime       = 0.0F;
    float interpolationTime = 0.1F;

    InterpolationMode mode = InterpolationMode::Linear;
    bool enabled           = true;

    float velocityX = 0.0F;
    float velocityY = 0.0F;

    void setTarget(float x, float y);
    void setTargetWithVelocity(float x, float y, float vx, float vy);
};
```

## System API

```cpp
#include "systems/InterpolationSystem.hpp"

class InterpolationSystem
{
public:
    void update(Registry& registry, float deltaTime);
};
```

## Interpolation Modes

### Linear Interpolation

Smoothly transitions from previous to target position over `interpolationTime`:

```cpp
InterpolationComponent interp;
interp.mode = InterpolationMode::Linear;
interp.interpolationTime = 0.1F;  
interp.setTarget(newX, newY);
```

**Formula**: `position = lerp(previous, target, t)` where `t = elapsedTime / interpolationTime`

### Extrapolation

Interpolates within the time window, then extrapolates beyond it using velocity:

```cpp
InterpolationComponent interp;
interp.mode = InterpolationMode::Extrapolate;

interp.setTargetWithVelocity(newX, newY, velocityX, velocityY);
```

**Behavior**:
- If `elapsedTime <= interpolationTime`: Interpolate to target
- If `elapsedTime > interpolationTime`: `position = target + velocity * (elapsedTime - interpolationTime)`

### None

No interpolation - snaps directly to target position:

```cpp
InterpolationComponent interp;
interp.mode = InterpolationMode::None;
```

## Usage Example

### Basic Setup

```cpp
#include "components/InterpolationComponent.hpp"
#include "components/TransformComponent.hpp"
#include "systems/InterpolationSystem.hpp"

EntityId player = registry.createEntity();
auto& transform = registry.emplace<TransformComponent>(player);
auto& interp    = registry.emplace<InterpolationComponent>(player);

interp.interpolationTime = 0.1F;  
interp.mode = InterpolationMode::Linear;

InterpolationSystem interpSystem;
while (running) {
    float deltaTime = clock.getDelta();
    interpSystem.update(registry, deltaTime);
}
```

### Handling Server Updates

```cpp
void onServerPositionUpdate(EntityId entity, float x, float y)
{
    if (registry.has<InterpolationComponent>(entity)) {
        auto& interp = registry.get<InterpolationComponent>(entity);
        interp.setTarget(x, y);
    }
}

void onServerStateUpdate(EntityId entity, float x, float y, float vx, float vy)
{
    if (registry.has<InterpolationComponent>(entity)) {
        auto& interp = registry.get<InterpolationComponent>(entity);
        interp.setTargetWithVelocity(x, y, vx, vy);
    }
}
```

### Complete Example

```cpp
#include "components/InterpolationComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/InterpolationSystem.hpp"

class NetworkManager
{
public:
    void handleServerUpdate(EntityId entity, float x, float y, float vx, float vy)
    {
        if (!registry.has<InterpolationComponent>(entity)) {
            auto& interp = registry.emplace<InterpolationComponent>(entity);
            interp.mode = InterpolationMode::Extrapolate;
            interp.interpolationTime = 0.1F;  
        }

        auto& interp = registry.get<InterpolationComponent>(entity);
        interp.setTargetWithVelocity(x, y, vx, vy);
    }

private:
    Registry& registry;
};

int main()
{
    Registry registry;
    InterpolationSystem interpSystem;
    NetworkManager netManager(registry);

    while (running) {
        netManager.processPackets();
        interpSystem.update(registry, deltaTime);

        renderSystem.update(registry);
    }
}
```

## Benefits

1. **Smooth Movement**: Eliminates jerky "teleportation" between server updates
2. **Latency Hiding**: Compensates for network delays
3. **Bandwidth Efficiency**: Reduces required server update rate
4. **Configurable**: Adjust `interpolationTime` based on server tick rate

## Best Practices

### Update Frequency

Match `interpolationTime` to your server's update rate:

```cpp
interp.interpolationTime = 0.1F;

interp.interpolationTime = 0.05F;
```

### Choosing Interpolation Mode

- **Linear**: Best for most cases, smooth and predictable
- **Extrapolate**: Use when server updates might be delayed (high latency)
- **None**: For server-authoritative entities that shouldn't be predicted

### Disabling Interpolation

```cpp
if (isLocalPlayer) {
    interp.enabled = false;
}
```

## Technical Details

### Time Clamping

The system clamps interpolation factor `t` to `[0, 1]` to prevent overshooting:

```cpp
float t = clamp(elapsedTime / interpolationTime, 0.0F, 1.0F);
```

### Linear Interpolation Formula

```cpp
position = start + (end - start) * t
```

### Extrapolation Formula

```cpp
position = target + velocity * (elapsedTime - interpolationTime)
```

## Performance

- **Complexity**: O(n) where n = entities with InterpolationComponent
- **Overhead**: ~10 floating-point operations per entity per frame
- **Memory**: 44 bytes per InterpolationComponent

## Integration with Other Systems

The InterpolationSystem directly modifies `TransformComponent`, which is then used by:
- RenderSystem (for drawing)
- PhysicsSystem (for collision detection)
- Other systems that read position

**Update Order**:
1. NetworkSystem (receive server updates)
2. InterpolationSystem (smooth positions)
3. RenderSystem (draw entities)
