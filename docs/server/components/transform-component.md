# TransformComponent

**Location:** `shared/include/components/TransformComponent.hpp` (planned)

The `TransformComponent` represents an entity's position (center) and orientation in the game world. It is the fundamental component for any entity that has a spatial presence.

---

## **Structure**

```cpp
struct TransformComponent {
    float x = 0.0F;        // World position X (center)
    float y = 0.0F;        // World position Y (center)
    float rotation = 0.0F; // Radians, counter-clockwise, 0 along +X axis
};
```

---

## **Fields**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `x` | `float` | `0.0F` | Horizontal position in world coordinates (entity center) |
| `y` | `float` | `0.0F` | Vertical position in world coordinates (entity center) |
| `rotation` | `float` | `0.0F` | Orientation angle in radians (counter-clockwise from +X) |

---

## **Coordinate System**

### World Space

* **X-axis:** Increases to the right →
* **Y-axis:** Increases downward ↓ (screen space convention)
* **Origin:** Entity center (not top-left corner)
* **Rotation:** 0 radians points along +X axis (right)

### Rotation Convention

```
    0.0 rad (0°) → →
    
π/2 rad (90°) ↓

π rad (180°) ← ←

3π/2 rad (270°) ↑
```

**Counter-clockwise** rotation from +X axis (standard mathematical convention).

---

## **Invariants**

* **Finite values** — `x`, `y`, and `rotation` must be finite (not NaN or infinite)
* **Rotation wrapping** — Rotation can be any real number; wrapping to \[0, 2π) is optional
* **Center-based** — Position represents the entity's center point

---

## **Usage**

### Creating an Entity with Transform

```cpp
Registry registry;
EntityId player = registry.createEntity();

// Create player at (100, 200) facing right (0 radians)
registry.emplace<TransformComponent>(player, TransformComponent{100.0F, 200.0F, 0.0F});
```

### Moving an Entity

```cpp
auto& transform = registry.get<TransformComponent>(player);
transform.x += 10.0F;  // Move 10 units right
transform.y += 5.0F;   // Move 5 units down
```

### Rotating an Entity

```cpp
auto& transform = registry.get<TransformComponent>(entity);
transform.rotation += 0.1F;  // Rotate 0.1 radians CCW

// Optional: Wrap to [0, 2π)
if (transform.rotation >= 2.0F * M_PI) {
    transform.rotation -= 2.0F * M_PI;
}
```

---

## **Systems That Use Transform**

### MovementSystem (Writer)

Integrates velocity to update position:

```cpp
for (EntityId id : registry.view<TransformComponent, VelocityComponent>()) {
    auto& transform = registry.get<TransformComponent>(id);
    auto& velocity = registry.get<VelocityComponent>(id);
    
    transform.x += velocity.vx * deltaTime;
    transform.y += velocity.vy * deltaTime;
}
```

### CollisionSystem (Reader)

Uses position for collision detection:

```cpp
for (EntityId id : registry.view<TransformComponent, HitboxComponent>()) {
    auto& transform = registry.get<TransformComponent>(id);
    auto& hitbox = registry.get<HitboxComponent>(id);
    
    // AABB bounds
    float left = transform.x + hitbox.offsetX - hitbox.width / 2.0F;
    float right = transform.x + hitbox.offsetX + hitbox.width / 2.0F;
    float top = transform.y + hitbox.offsetY - hitbox.height / 2.0F;
    float bottom = transform.y + hitbox.offsetY + hitbox.height / 2.0F;
}
```

### RenderSystem (Reader)

Uses position and rotation for sprite rendering:

```cpp
for (EntityId id : registry.view<TransformComponent, SpriteComponent>()) {
    auto& transform = registry.get<TransformComponent>(id);
    auto& sprite = registry.get<SpriteComponent>(id);
    
    sprite.setPosition(transform.x, transform.y);
    sprite.setRotation(transform.rotation * 180.0F / M_PI);  // Convert to degrees for SFML
}
```

### PlayerSystem (Writer)

Updates player position based on input:

```cpp
auto& transform = registry.get<TransformComponent>(playerId);
auto& input = registry.get<PlayerInputComponent>(playerId);

// Server-authoritative position
transform.x = input.x;  // Or apply validated movement
transform.rotation = input.angle;
```

---

## **Networking**

### Replication

* **On Spawn:** Full `TransformComponent` (x, y, rotation) sent to all clients
* **On Update:** Delta updates when values change significantly
* **Frequency:** Every tick for moving entities, on-demand for static entities

### Delta Compression

Only send transform when it has changed by a threshold:

```cpp
constexpr float POSITION_THRESHOLD = 0.5F;
constexpr float ROTATION_THRESHOLD = 0.01F;

if (abs(newTransform.x - lastSent.x) > POSITION_THRESHOLD ||
    abs(newTransform.y - lastSent.y) > POSITION_THRESHOLD ||
    abs(newTransform.rotation - lastSent.rotation) > ROTATION_THRESHOLD) {
    // Send delta update
}
```

### Binary Format

```cpp
// Packed as 12 bytes:
// [float x (4 bytes)]
// [float y (4 bytes)]
// [float rotation (4 bytes)]
```

---

## **Examples**

### Player Entity

```cpp
EntityId player = registry.createEntity();
registry.emplace<TransformComponent>(player, TransformComponent{100.0F, 200.0F, 0.0F});
registry.emplace<VelocityComponent>(player, VelocityComponent{0.0F, 0.0F});
registry.emplace<HitboxComponent>(player, HitboxComponent{32.0F, 16.0F});
```

### Enemy Monster

```cpp
EntityId monster = registry.createEntity();
registry.emplace<TransformComponent>(monster, TransformComponent{800.0F, 300.0F, M_PI});  // Facing left
registry.emplace<MovementComponent>(monster, MovementComponent::sine(100.0F, 50.0F, 1.5F));
```

### Missile

```cpp
EntityId missile = registry.createEntity();

// Spawn at player position, inherit rotation
auto& playerTransform = registry.get<TransformComponent>(player);
registry.emplace<TransformComponent>(missile, TransformComponent{
    playerTransform.x,
    playerTransform.y,
    playerTransform.rotation
});
```

---

## **Design Decisions**

### Why Center-Based Origin?

✅ **Symmetric sprites** — No offset calculations for centered visuals\
✅ **Rotation simplicity** — Rotate around center, not corner\
✅ **Collision math** — AABB calculations are clearer with center coords

### Why Y-Down?

✅ **Screen space convention** — Matches SFML and most 2D engines\
✅ **Simpler rendering** — No coordinate flip needed\
✅ **UI consistency** — Text and UI elements naturally flow downward

### Why Radians?

✅ **Math library compatibility** — `sin()`, `cos()` use radians\
✅ **No conversion overhead** — Direct use in physics calculations\
✅ **Precision** — Floating-point radians avoid degree rounding

---

## **Validation**

### Sanity Checks

```cpp
bool isValid(const TransformComponent& t) {
    return std::isfinite(t.x) &&
           std::isfinite(t.y) &&
           std::isfinite(t.rotation);
}
```

### Bounds Clamping (Optional)

```cpp
// Keep entities within world bounds
constexpr float WORLD_MIN_X = 0.0F;
constexpr float WORLD_MAX_X = 1920.0F;
constexpr float WORLD_MIN_Y = 0.0F;
constexpr float WORLD_MAX_Y = 1080.0F;

transform.x = std::clamp(transform.x, WORLD_MIN_X, WORLD_MAX_X);
transform.y = std::clamp(transform.y, WORLD_MIN_Y, WORLD_MAX_Y);
```

---

## **Performance Considerations**

* **Cache-friendly** — 12 bytes, fits in cache lines
* **No pointers** — Plain Old Data (POD), trivially copyable
* **Direct access** — No getters/setters overhead

---

## **Related Components**

* [VelocityComponent](velocity-component.md) — Determines rate of position change
* [HitboxComponent](hitbox-component.md) — Collision bounds relative to transform
* [SpriteComponent](sprite-component.md) — Visual representation at transform position

---

## **Related Systems**

* **MovementSystem** — Integrates velocity into transform
* **CollisionSystem** — Uses transform for spatial queries
* **RenderSystem** — Draws sprites at transform position
* **NetworkSystem** — Replicates transform to clients
