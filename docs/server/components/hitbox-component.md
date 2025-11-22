# HitboxComponent

**Location:** `shared/include/components/HitboxComponent.hpp` (planned)

The `HitboxComponent` defines an Axis-Aligned Bounding Box (AABB) for collision detection. It uses an AABB with optional offset relative to the entity center.

---

## **Structure**

```cpp
struct HitboxComponent {
    float width = 0.0F;        // AABB width (world units)
    float height = 0.0F;       // AABB height (world units)
    float offsetX = 0.0F;      // Local offset from center X
    float offsetY = 0.0F;      // Local offset from center Y
    bool isTrigger = false;    // If true, registers overlaps without blocking
};
```

---

## **Fields**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `width` | `float` | `0.0F` | Width of the bounding box |
| `height` | `float` | `0.0F` | Height of the bounding box |
| `offsetX` | `float` | `0.0F` | Horizontal offset from entity center |
| `offsetY` | `float` | `0.0F` | Vertical offset from entity center |
| `isTrigger` | `bool` | `false` | If true, detects overlaps but doesn't block movement |

---

## **Invariants**

* `width >= 0`
* `height >= 0  `
* AABB is **axis-aligned** — rotation does not affect collision box size
* Collision detection is simplified by ignoring rotation

---

## **AABB Calculation**

Given a `TransformComponent` and `HitboxComponent`:

```cpp
float centerX = transform.x + hitbox.offsetX;
float centerY = transform.y + hitbox.offsetY;

float left = centerX - hitbox.width / 2.0F;
float right = centerX + hitbox.width / 2.0F;
float top = centerY - hitbox.height / 2.0F;
float bottom = centerY + hitbox.height / 2.0F;
```

---

## **Collision Detection**

### AABB vs AABB

```cpp
bool checkCollision(const AABB& a, const AABB& b) {
    return (a.left < b.right &&
            a.right > b.left &&
            a.top < b.bottom &&
            a.bottom > b.top);
}
```

### CollisionSystem Example

```cpp
for (EntityId a : registry.view<TransformComponent, HitboxComponent>()) {
    for (EntityId b : registry.view<TransformComponent, HitboxComponent>()) {
        if (a == b) continue;
        
        auto [transA, hitA] = registry.get<TransformComponent, HitboxComponent>(a);
        auto [transB, hitB] = registry.get<TransformComponent, HitboxComponent>(b);
        
        if (checkAABBCollision(transA, hitA, transB, hitB)) {
            // Handle collision
        }
    }
}
```

---

## **Trigger vs Solid**

* **`isTrigger = false`** (default): Solid collision, can block movement
* **`isTrigger = true`**: Detects overlap for events (e.g., pickups, projectiles)

---

## **Examples**

### Player Hitbox

```cpp
registry.emplace<HitboxComponent>(player, HitboxComponent{
    32.0F,   // width
    16.0F,   // height
    0.0F,    // offsetX (centered)
    0.0F,    // offsetY (centered)
    false    // solid
});
```

### Missile Trigger

```cpp
registry.emplace<HitboxComponent>(missile, HitboxComponent{
    8.0F,    // width
    4.0F,    // height
    0.0F,    // offsetX
    0.0F,    // offsetY
    true     // trigger (doesn't block, just detects hits)
});
```

---

## **Systems**

* **CollisionSystem** — Detects and resolves collisions
* **MissileSystem** — Uses triggers for hit detection
* **DestructionSystem** — Handles collision damage

---

## **Networking**

* **Optional replication** — May be sent for client-side prediction
* **Not critical for MVP** — Server handles all collision logic

---

## **Related**

* [TransformComponent](transform-component.md)
* [MissileComponent](missile-component.md)
* [HealthComponent](health-component.md)
