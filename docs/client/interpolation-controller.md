# InterpolationController

## Overview

The `InterpolationController` is a helper class that provides clean, safe management of `InterpolationComponent` lifecycle. It ensures that elapsed time is properly reset when new targets are set, handles mode transitions cleanly, and provides utility methods for common interpolation tasks.

## Purpose

While `InterpolationComponent` stores interpolation state and `InterpolationSystem` performs the actual interpolation calculations, the `InterpolationController` provides a high-level API that:

- **Automatically resets elapsed time** when setting new targets
- **Handles mode transitions** (Linear, Extrapolate, None) with proper snapping
- **Manages enable/disable state** cleanly
- **Provides utility queries** like progress and target distance
- **Validates inputs** and handles edge cases (dead entities, missing components)

## Architecture

```
┌─────────────────────────────────┐
│   InterpolationController       │
│  (Helper/Facade)                │
│  - setTarget()                  │
│  - setMode()                    │
│  - enable/disable()             │
│  - isAtTarget()                 │
│  - getProgress()                │
└────────────┬────────────────────┘
             │ manipulates
             ▼
┌─────────────────────────────────┐
│   InterpolationComponent        │
│  (Data)                         │
│  - previousX/Y                  │
│  - targetX/Y                    │
│  - elapsedTime                  │
│  - mode, enabled                │
└────────────┬────────────────────┘
             │ read by
             ▼
┌─────────────────────────────────┐
│   InterpolationSystem           │
│  (Logic)                        │
│  - update()                     │
│  - applies interpolation        │
└─────────────────────────────────┘
```

## Core Features

### 1. Target Management with Automatic Reset

The controller ensures `elapsedTime` is always reset when setting new targets:

```cpp
InterpolationController controller;
EntityId entity = registry.createEntity();
registry.emplace<InterpolationComponent>(entity);
registry.emplace<TransformComponent>(entity);

// Set target - automatically resets elapsed time to 0
controller.setTarget(registry, entity, 100.0F, 200.0F);

// For extrapolation mode, include velocity
controller.setTargetWithVelocity(registry, entity, 100.0F, 200.0F, 50.0F, 50.0F);
```

**Why this matters**: Forgetting to reset `elapsedTime` causes the interpolation to start mid-way or complete instantly. The controller prevents this bug.

### 2. Clean Mode Transitions

```cpp
// Switch to linear interpolation
controller.setMode(registry, entity, InterpolationMode::Linear);

// Switch to extrapolation (requires velocity to be set)
controller.setMode(registry, entity, InterpolationMode::Extrapolate);

// Disable interpolation and snap to target immediately
controller.setMode(registry, entity, InterpolationMode::None);
```

When setting mode to `None`, the controller automatically snaps the `TransformComponent` to the target position.

### 3. Enable/Disable with Snapping

```cpp
// Enable interpolation
controller.enable(registry, entity);

// Disable interpolation and snap to target
controller.disable(registry, entity);
```

Disabling interpolation also snaps the entity to its target position, ensuring consistent state.

### 4. Clamping to Target

```cpp
// Force entity to target position, set elapsed to max, and disable
controller.clampToTarget(registry, entity);
```

Useful when you want to immediately complete an interpolation (e.g., on level transition or teleport).

### 5. Reset to Defaults

```cpp
// Clear all interpolation state
controller.reset(registry, entity);
```

Resets to:
- `previousX/Y = 0.0F`
- `targetX/Y = 0.0F`
- `elapsedTime = 0.0F`
- `velocityX/Y = 0.0F`
- `mode = Linear`
- `enabled = true`

### 6. Utility Queries

```cpp
// Check if entity is at target (within threshold)
if (controller.isAtTarget(registry, entity, 0.5F)) {
    // Entity reached destination
}

// Get interpolation progress (0.0 to 1.0)
float progress = controller.getProgress(registry, entity);
if (progress >= 0.9F) {
    // Almost done, trigger arrival animation
}
```

## Usage Examples

### Example 1: Network Entity Replication

```cpp
void ReplicationSystem::onServerSnapshot(const Snapshot& snapshot) {
    for (const auto& entityData : snapshot.entities) {
        EntityId localEntity = findOrCreateLocalEntity(entityData.serverId);

        auto& interp = registry.get<InterpolationComponent>(localEntity);

        // Set new target from server state
        // Controller automatically resets elapsed time
        controller.setTargetWithVelocity(
            registry,
            localEntity,
            entityData.x,
            entityData.y,
            entityData.vx,
            entityData.vy
        );

        // Use extrapolation for smoother motion when packets are delayed
        controller.setMode(registry, localEntity, InterpolationMode::Extrapolate);
        controller.setInterpolationTime(registry, localEntity, 0.1F); // 100ms
    }
}
```

### Example 2: Cinematic Camera Movement

```cpp
void CinematicSystem::moveCameraToPoint(EntityId camera, float x, float y) {
    // Set camera target
    controller.setTarget(registry, camera, x, y);
    controller.setMode(registry, camera, InterpolationMode::Linear);
    controller.setInterpolationTime(registry, camera, 2.0F); // 2 second smooth pan

    // Wait for completion in update loop
    if (controller.isAtTarget(registry, camera)) {
        onCameraArrived();
    }
}
```

### Example 3: UI Animation with Progress

```cpp
void UISystem::animateButton(EntityId button) {
    float progress = controller.getProgress(registry, button);

    // Update button visual based on interpolation progress
    if (progress < 0.2F) {
        // Start of animation - ease in
        setButtonOpacity(button, progress * 5.0F);
    } else if (progress > 0.8F) {
        // End of animation - ease out
        setButtonOpacity(button, 1.0F - (progress - 0.8F) * 5.0F);
    } else {
        setButtonOpacity(button, 1.0F);
    }
}
```

### Example 4: Projectile with Course Correction

```cpp
void ProjectileSystem::updateHoming(EntityId projectile, EntityId target) {
    auto& targetTransform = registry.get<TransformComponent>(target);

    // Check if we need to update target
    if (!controller.isAtTarget(registry, projectile, 5.0F)) {
        // Recalculate velocity toward moving target
        float dx = targetTransform.x - /* current projectile x */;
        float dy = targetTransform.y - /* current projectile y */;
        float speed = 300.0F;
        float distance = std::sqrt(dx * dx + dy * dy);
        float vx = (dx / distance) * speed;
        float vy = (dy / distance) * speed;

        // Update target with new velocity
        controller.setTargetWithVelocity(
            registry,
            projectile,
            targetTransform.x,
            targetTransform.y,
            vx,
            vy
        );
    }
}
```

## API Reference

### setTarget()

```cpp
void setTarget(Registry& registry, EntityId entityId, float x, float y);
```

Sets new interpolation target and resets `elapsedTime` to 0. Automatically stores current target as previous position.

**Parameters:**
- `registry` - The ECS registry
- `entityId` - Entity with InterpolationComponent
- `x, y` - New target position

**Safety:** No-op if entity is dead or missing InterpolationComponent.

---

### setTargetWithVelocity()

```cpp
void setTargetWithVelocity(Registry& registry, EntityId entityId,
                           float x, float y, float vx, float vy);
```

Sets new target with velocity for extrapolation mode. Calls `setTarget()` internally (resets elapsed time) then sets velocity.

**Parameters:**
- `registry` - The ECS registry
- `entityId` - Entity with InterpolationComponent
- `x, y` - New target position
- `vx, vy` - Velocity for extrapolation beyond target

**Use case:** Network replication with extrapolation when packets are delayed.

---

### setMode()

```cpp
void setMode(Registry& registry, EntityId entityId, InterpolationMode mode);
```

Changes interpolation mode. If mode is `None`, automatically snaps entity to target position.

**Modes:**
- `InterpolationMode::Linear` - Smooth lerp from previous to target
- `InterpolationMode::Extrapolate` - Lerp to target, then extrapolate using velocity
- `InterpolationMode::None` - Instant snap to target (disables interpolation)

---

### enable()

```cpp
void enable(Registry& registry, EntityId entityId);
```

Enables interpolation. The `InterpolationSystem` will process this entity.

---

### disable()

```cpp
void disable(Registry& registry, EntityId entityId);
```

Disables interpolation and snaps entity to target position. The `InterpolationSystem` will skip this entity.

**Use case:** When entity stops moving or enters a non-interpolated state.

---

### clampToTarget()

```cpp
void clampToTarget(Registry& registry, EntityId entityId);
```

Forces entity to target position, sets `elapsedTime` to `interpolationTime`, and disables interpolation.

**Use case:** Forcefully complete an interpolation (teleport, level transition, cutscene).

---

### reset()

```cpp
void reset(Registry& registry, EntityId entityId);
```

Resets all interpolation state to defaults:
- Position/target/velocity to 0
- Elapsed time to 0
- Mode to Linear
- Enabled to true

---

### setInterpolationTime()

```cpp
void setInterpolationTime(Registry& registry, EntityId entityId, float time);
```

Sets the duration of interpolation in seconds.

**Validation:** Rejects values <= 0.

**Typical values:**
- `0.05F` - 50ms, very fast (responsive but visible snapping)
- `0.1F` - 100ms, standard for networked entities
- `0.2F` - 200ms, smooth but adds latency
- `1.0F+` - Cinematic camera movements

---

### isAtTarget()

```cpp
bool isAtTarget(Registry& registry, EntityId entityId, float threshold = 0.01F) const;
```

Checks if entity's current position is within threshold distance of target.

**Returns:** `true` if distance <= threshold, `false` otherwise.

**Use case:** Trigger events when entity reaches destination.

---

### getProgress()

```cpp
float getProgress(Registry& registry, EntityId entityId) const;
```

Returns interpolation progress as a value from 0.0 (start) to 1.0 (complete).

**Returns:** Clamped to [0.0, 1.0] range.

**Use case:** Drive animations, fade effects, or UI based on interpolation progress.

## Edge Case Handling

The controller is designed to be safe and robust:

| Scenario | Behavior |
|----------|----------|
| Dead entity | No-op, no error |
| Missing InterpolationComponent | No-op, no error |
| Missing TransformComponent (for snap operations) | No-op, no error |
| Negative interpolation time | Rejected, no change |
| Zero interpolation time | Rejected, no change |

This allows you to call controller methods without extensive validation checks.

## Best Practices

### ✅ DO

- Use `setTarget()` instead of manually setting `targetX/targetY` and resetting `elapsedTime`
- Use `disable()` or `clampToTarget()` when you want to stop interpolation cleanly
- Use `isAtTarget()` to trigger arrival events/animations
- Use `getProgress()` to drive easing effects

### ❌ DON'T

- Don't manually set `elapsedTime` after calling `setTarget()` (controller already resets it)
- Don't use `InterpolationMode::Extrapolate` without setting velocity
- Don't call `setInterpolationTime()` every frame (set once when creating entity)

## Performance Considerations

- **Lightweight**: All methods are simple direct manipulations of component data
- **No allocations**: Pure stack-based operations
- **Entity validation**: Checks `isAlive()` before accessing components (minimal overhead)
- **Cache-friendly**: Operates on contiguous component storage via Registry

## Integration with Other Systems

### With InterpolationSystem

The `InterpolationSystem` reads the component state that the controller manages:

```cpp
// Set up interpolation with controller
controller.setTarget(registry, entity, 100.0F, 200.0F);
controller.setMode(registry, entity, InterpolationMode::Linear);

// InterpolationSystem automatically processes this entity every frame
interpolationSystem.update(registry, deltaTime);
```

### With ReplicationSystem

```cpp
void ReplicationSystem::update(Registry& registry, float deltaTime) {
    for (const auto& update : serverUpdates) {
        EntityId entity = localEntityMap[update.serverId];

        // Use controller to manage interpolation
        controller.setTargetWithVelocity(
            registry, entity,
            update.x, update.y,
            update.vx, update.vy
        );
    }
}
```

### With CameraSystem

```cpp
void CameraSystem::followEntity(EntityId camera, EntityId target) {
    auto& targetTransform = registry.get<TransformComponent>(target);

    // Smooth camera follow using interpolation
    controller.setTarget(registry, camera, targetTransform.x, targetTransform.y);
    controller.setInterpolationTime(registry, camera, 0.15F);
}
```

## Testing

The InterpolationController has comprehensive test coverage (24 tests):

- Target setting and elapsed time reset
- Mode transitions and snapping behavior
- Enable/disable with proper snapping
- Clamping to target
- Reset functionality
- Interpolation time validation
- Distance and progress queries
- Edge cases (dead entities, missing components)

Run tests:
```bash
make test_client
# Or specifically:
./rtype_client_tests --gtest_filter="InterpolationController*"
```

## Common Patterns

### Pattern 1: Two-State Interpolation

For UI elements that toggle between two positions:

```cpp
bool isOpen = false;

void toggleMenu(EntityId menu) {
    if (isOpen) {
        controller.setTarget(registry, menu, -200.0F, 0.0F); // Off-screen
    } else {
        controller.setTarget(registry, menu, 0.0F, 0.0F);    // On-screen
    }
    controller.setInterpolationTime(registry, menu, 0.3F);
    isOpen = !isOpen;
}
```

### Pattern 2: Chained Interpolations

For patrol or waypoint movement:

```cpp
std::vector<sf::Vector2f> waypoints = { {100, 100}, {200, 100}, {200, 200}, {100, 200} };
int currentWaypoint = 0;

void updatePatrol(EntityId entity) {
    if (controller.isAtTarget(registry, entity, 5.0F)) {
        currentWaypoint = (currentWaypoint + 1) % waypoints.size();
        auto& wp = waypoints[currentWaypoint];
        controller.setTarget(registry, entity, wp.x, wp.y);
    }
}
```

### Pattern 3: Progress-Driven Effects

Use progress to drive visual effects:

```cpp
void updateTeleportEffect(EntityId entity) {
    float progress = controller.getProgress(registry, entity);

    // Fade out in first half, fade in during second half
    float alpha = progress < 0.5F
        ? 1.0F - (progress * 2.0F)  // 1.0 -> 0.0
        : (progress - 0.5F) * 2.0F; // 0.0 -> 1.0

    setSpriteAlpha(entity, alpha);
}
```

## Troubleshooting

### Issue: Entity not interpolating

**Check:**
1. Is `enabled` true? Use `controller.enable()`
2. Is `mode` set to `Linear` or `Extrapolate`? (Not `None`)
3. Is `InterpolationSystem` running every frame?
4. Is `interpolationTime` > 0?

### Issue: Entity snapping instead of smooth movement

**Check:**
1. Is `elapsedTime` being reset on every frame? (Don't call `setTarget()` every frame)
2. Is `interpolationTime` too small? Try 0.1F or larger
3. Is deltaTime consistent?

### Issue: Extrapolation not working

**Check:**
1. Did you use `setTargetWithVelocity()` (not just `setTarget()`)?
2. Is velocity non-zero?
3. Is mode set to `InterpolationMode::Extrapolate`?

## See Also

- [InterpolationComponent](../components/interpolation-component.md) - The underlying component
- [InterpolationSystem](../systems/interpolation-system.md) - The system that performs calculations
- [ReplicationSystem](replication-system.md) - Uses interpolation for network entities
- [CameraSystem](camera-system.md) - Uses interpolation for smooth camera movement
