# InterpolationComponent

## Overview

The `InterpolationComponent` is a networking and rendering component in the R-Type client that enables smooth visual interpolation of entity positions between network updates. It eliminates the visual stutter that would occur if entities only moved when network packets arrived, creating fluid motion even at low network tick rates.

## Purpose and Design Philosophy

Network games face a fundamental challenge: servers typically send updates at 20-60 Hz, but clients render at 60-144+ FPS. Without interpolation, entities would appear to "teleport" between positions:

1. **Visual Smoothing**: Entities move smoothly between known positions
2. **Network Independence**: Visual updates independent of packet timing
3. **Latency Hiding**: Masks network jitter and packet loss
4. **Extrapolation**: Predicts position when packets are late
5. **Velocity Awareness**: Uses velocity for better extrapolation

The component stores the interpolation state while the `InterpolationSystem` performs the actual position calculations.

## Component Structure

```cpp
enum class InterpolationMode : std::uint8_t
{
    None,
    Linear,
    Extrapolate
};

struct InterpolationComponent
{
    float previousX = 0.0F;
    float previousY = 0.0F;

    float targetX = 0.0F;
    float targetY = 0.0F;

    float elapsedTime       = 0.0F;
    float interpolationTime = 0.1F;

    InterpolationMode mode = InterpolationMode::Linear;
    bool enabled           = true;

    float velocityX = 0.0F;
    float velocityY = 0.0F;

    float maxExtrapolationTime = 0.2F;

    void setTarget(float x, float y);
    void setTargetWithVelocity(float x, float y, float vx, float vy);
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `previousX` | `float` | `0.0F` | Starting X position for interpolation. |
| `previousY` | `float` | `0.0F` | Starting Y position for interpolation. |
| `targetX` | `float` | `0.0F` | Target X position to interpolate toward. |
| `targetY` | `float` | `0.0F` | Target Y position to interpolate toward. |
| `elapsedTime` | `float` | `0.0F` | Time elapsed since last target update. |
| `interpolationTime` | `float` | `0.1F` | Expected time between updates (100ms default). |
| `mode` | `InterpolationMode` | `Linear` | Interpolation algorithm to use. |
| `enabled` | `bool` | `true` | Whether interpolation is active. |
| `velocityX` | `float` | `0.0F` | Entity velocity X for extrapolation. |
| `velocityY` | `float` | `0.0F` | Entity velocity Y for extrapolation. |
| `maxExtrapolationTime` | `float` | `0.2F` | Maximum time to extrapolate before freezing. |

### Interpolation Modes

```cpp
enum class InterpolationMode : std::uint8_t
{
    None,        // No interpolation, snap to target
    Linear,      // Linear interpolation between previous and target
    Extrapolate  // Extrapolate beyond target using velocity
};
```

## Methods

### `setTarget(float x, float y)`

Sets a new target position, moving current target to previous.

```cpp
void InterpolationComponent::setTarget(float x, float y)
{
    previousX = targetX;
    previousY = targetY;
    targetX = x;
    targetY = y;
    elapsedTime = 0.0F;
}
```

### `setTargetWithVelocity(float x, float y, float vx, float vy)`

Sets target with velocity for better extrapolation.

```cpp
void InterpolationComponent::setTargetWithVelocity(float x, float y, float vx, float vy)
{
    setTarget(x, y);
    velocityX = vx;
    velocityY = vy;
}
```

## Usage Patterns

### Initial Entity Setup

```cpp
Entity createNetworkedEnemy(Registry& registry, float x, float y)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    registry.addComponent(entity, SpriteComponent::create("enemy"));
    
    // Add interpolation for smooth rendering
    InterpolationComponent interp;
    interp.previousX = x;
    interp.previousY = y;
    interp.targetX = x;
    interp.targetY = y;
    interp.interpolationTime = 1.0F / 20.0F;  // 20 Hz server tick rate
    interp.mode = InterpolationMode::Linear;
    registry.addComponent(entity, interp);
    
    return entity;
}
```

### Receiving Network Updates

```cpp
void SnapshotParser::updateEntity(Registry& registry, Entity entity,
                                    const EntitySnapshot& snapshot)
{
    auto& interp = registry.getComponent<InterpolationComponent>(entity);
    
    // Set new target from server data
    if (snapshot.hasVelocity) {
        interp.setTargetWithVelocity(
            snapshot.x, snapshot.y,
            snapshot.velocityX, snapshot.velocityY
        );
        interp.mode = InterpolationMode::Extrapolate;
    } else {
        interp.setTarget(snapshot.x, snapshot.y);
        interp.mode = InterpolationMode::Linear;
    }
}
```

## InterpolationSystem Logic

The system updates visual positions each frame:

```cpp
void InterpolationSystem::update(Registry& registry, float deltaTime)
{
    auto view = registry.view<InterpolationComponent, TransformComponent>();
    
    for (auto entity : view) {
        auto& interp = view.get<InterpolationComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        
        if (!interp.enabled) {
            transform.x = interp.targetX;
            transform.y = interp.targetY;
            continue;
        }
        
        interp.elapsedTime += deltaTime;
        
        switch (interp.mode) {
            case InterpolationMode::None:
                transform.x = interp.targetX;
                transform.y = interp.targetY;
                break;
                
            case InterpolationMode::Linear:
                applyLinearInterpolation(transform, interp);
                break;
                
            case InterpolationMode::Extrapolate:
                applyExtrapolation(transform, interp, deltaTime);
                break;
        }
    }
}

void InterpolationSystem::applyLinearInterpolation(TransformComponent& transform,
                                                     const InterpolationComponent& interp)
{
    // Calculate interpolation factor (0.0 to 1.0)
    float t = std::min(1.0F, interp.elapsedTime / interp.interpolationTime);
    
    // Linear interpolation: previous + t * (target - previous)
    transform.x = interp.previousX + t * (interp.targetX - interp.previousX);
    transform.y = interp.previousY + t * (interp.targetY - interp.previousY);
}

void InterpolationSystem::applyExtrapolation(TransformComponent& transform,
                                               const InterpolationComponent& interp,
                                               float deltaTime)
{
    float t = interp.elapsedTime / interp.interpolationTime;
    
    if (t <= 1.0F) {
        // Still interpolating to target
        transform.x = interp.previousX + t * (interp.targetX - interp.previousX);
        transform.y = interp.previousY + t * (interp.targetY - interp.previousY);
    } else {
        // Extrapolating beyond target
        float extraTime = interp.elapsedTime - interp.interpolationTime;
        
        if (extraTime < interp.maxExtrapolationTime) {
            // Continue movement using velocity
            transform.x = interp.targetX + interp.velocityX * extraTime;
            transform.y = interp.targetY + interp.velocityY * extraTime;
        }
        // Beyond max extrapolation time: freeze at last known good position
    }
}
```

## Visual Comparison

```
Without Interpolation (20 Hz updates, 60 FPS render):
Frame: | 1 | 2 | 3 |...| 1 | 2 | 3 |...| 1 | 2 | 3 |
       |___|___|___|   |___|___|___|   |___|___|___|
       [===]           [===]           [===]
       └── Position only updates every 3 frames (teleporting)

With Interpolation:
Frame: | 1 | 2 | 3 |...| 1 | 2 | 3 |...| 1 | 2 | 3 |
       [=][==][===]   [==][===][=]   [===][=][==]
       └── Smooth motion every frame
```

## Interpolation Time Configuration

Based on server tick rate:

| Server Tick Rate | interpolationTime | Notes |
|-----------------|-------------------|-------|
| 60 Hz | 0.0167s (16.7ms) | Competitive games |
| 30 Hz | 0.0333s (33.3ms) | Standard games |
| 20 Hz | 0.05s (50ms) | Default, good for most |
| 10 Hz | 0.1s (100ms) | Low bandwidth games |

## Player vs Remote Entity Interpolation

```cpp
void configureInterpolation(Registry& registry, Entity entity, bool isLocalPlayer)
{
    auto& interp = registry.getComponent<InterpolationComponent>(entity);
    
    if (isLocalPlayer) {
        // Local player uses prediction, minimal interpolation
        interp.enabled = false;  // Transform updated directly by input
    } else {
        // Remote entities use full interpolation
        interp.enabled = true;
        interp.interpolationTime = m_serverTickInterval;
        interp.mode = InterpolationMode::Extrapolate;
        interp.maxExtrapolationTime = m_serverTickInterval * 4.0F;
    }
}
```

## Jitter Handling

For network jitter compensation:

```cpp
void adaptInterpolationTime(InterpolationComponent& interp, float actualInterval)
{
    // Exponential moving average of actual packet intervals
    const float ALPHA = 0.1F;
    interp.interpolationTime = interp.interpolationTime * (1.0F - ALPHA) + 
                                actualInterval * ALPHA;
    
    // Clamp to reasonable bounds
    interp.interpolationTime = std::clamp(interp.interpolationTime, 0.016F, 0.2F);
}
```

## Best Practices

1. **Match Server Rate**: Set `interpolationTime` to match server tick interval
2. **Use Velocity**: Enable extrapolation with velocity for moving entities
3. **Limit Extrapolation**: Cap `maxExtrapolationTime` to avoid visual artifacts
4. **Disable for Local**: Local player doesn't need interpolation (uses prediction)
5. **Handle Teleports**: Large position changes should bypass interpolation

## Teleport Detection

```cpp
void handlePositionUpdate(InterpolationComponent& interp, float newX, float newY)
{
    float dx = newX - interp.targetX;
    float dy = newY - interp.targetY;
    float distSquared = dx * dx + dy * dy;
    
    constexpr float TELEPORT_THRESHOLD_SQ = 100.0F * 100.0F;  // 100 units
    
    if (distSquared > TELEPORT_THRESHOLD_SQ) {
        // Teleport: snap immediately
        interp.previousX = newX;
        interp.previousY = newY;
        interp.targetX = newX;
        interp.targetY = newY;
        interp.elapsedTime = interp.interpolationTime;  // Skip interpolation
    } else {
        // Normal update
        interp.setTarget(newX, newY);
    }
}
```

## Related Components

- [TransformComponent](../server/components/transform-component.md) - Actual render position
- [VelocityComponent](../server/components/velocity-component.md) - Movement velocity

## Related Systems

- [InterpolationSystem](../systems/interpolation-system.md) - Processes interpolation
- [SnapshotParser](snapshot-parser.md) - Provides new target positions
- [ReplicationSystem](../systems/replication-system.md) - Network entity updates
