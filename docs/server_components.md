# Server MVP Components (ECS)

This document defines the minimal set of server-side ECS components required for the MVP and how they interact. Each component is a plain data struct (POD), compatible with the shared ECS `Registry` and designed to be easily reused by the client where relevant.

Assumptions and conventions:
- Coordinates use world units; `x` increases to the right, `y` increases downward (screen space).
- Transform origin is the entity center.
- Angles are in radians, counter-clockwise (0 aligned with +X).
- Time values are expressed in seconds.
- `EntityId` is a 32-bit unsigned integer.

Placement: Components that are reusable between client and server should live under `shared/include/components/` and be re-exported via `components/Components.hpp`. Server-only components may live under a future `server/include/components/` if needed.

Goals:
- Clean, minimal, and deterministic data layout.
- Straightforward serialization for networking.
- Clear ownership and responsibility across systems.

---

## Component Reference

Each reference includes fields, invariants, typical producers/consumers (systems), and networking notes.

### TransformComponent

Represents an entity's position (center) and orientation.

```cpp
struct TransformComponent {
    float x = 0.0F;        // world position X (center)
    float y = 0.0F;        // world position Y (center)
    float rotation = 0.0F; // radians, CCW, 0 along +X
};
```

- Invariants: position and rotation are finite values; rotation can be any real number (wrap as needed).
- Systems: MovementSystem (writes x,y), PlayerSystem/AI/MissileSystem (may write rotation), Networking (reads to replicate).
- Networking: replicated to clients on spawn and deltas when changed.

### VelocityComponent

Represents an entity's linear velocity.

```cpp
struct VelocityComponent {
    float vx = 0.0F; // velocity on X (units/sec)
    float vy = 0.0F; // velocity on Y (units/sec)
};
```

- Invariants: finite values.
- Systems: MovementSystem integrates `TransformComponent` using `vx`, `vy` and delta-time.
- Networking: usually not replicated; clients can infer from Transform deltas.

### HitboxComponent

Axis-Aligned Bounding Box (AABB) for collision checks, with optional offset relative to the entity center.

```cpp
struct HitboxComponent {
    float width = 0.0F;   // AABB width  (world units)
    float height = 0.0F;  // AABB height (world units)
    float offsetX = 0.0F; // local offset from center X
    float offsetY = 0.0F; // local offset from center Y
    bool isTrigger = false; // if true, registers overlaps without blocking
};
```

- Invariants: `width >= 0`, `height >= 0`. AABB is axis-aligned; rotation does not affect size (simplifies checks).
- Systems: CollisionSystem (reads/writes trigger state as needed), MissileSystem (uses triggers), DestructionSystem (via damage events).
- Networking: may be replicated if client-side prediction needs it; optional for MVP.

### HealthComponent

Health pool for damageable entities.

```cpp
#include <cstdint>

struct HealthComponent {
    std::int32_t current = 1; // current HP
    std::int32_t max = 1;     // maximum HP
    bool invulnerable = false; // if true, ignores incoming damage
};
```

- Invariants: `0 <= current <= max`, `max >= 0`.
- Systems: DestructionSystem (removes entity at `current <= 0`), UI/Networking (reads for health bars/updates), CollisionSystem (applies damage).
- Networking: replicate `current` (and optionally `max`) on change.

### PlayerInputComponent

Latest validated input state for a player entity. The server remains authoritative; these values are used for reconciliation and orientation.

```cpp
#include <cstdint>

struct PlayerInputComponent {
    // Client-reported position estimate (center), used for reconciliation/smoothing
    float x = 0.0F;
    float y = 0.0F;

    // Client aim/ship angle in radians (0 along +X, CCW)
    float angle = 0.0F;

    // For reconciliation/ordering (optional)
    std::uint16_t sequenceId = 0; // last processed input seq
};
```

- Invariants: finite values. `sequenceId` monotonically increases per client.
- Systems: PlayerSystem (consumes), Networking (produces from inbound UDP), MovementSystem may consult.
- Networking: never broadcast to other clients; only received from the owning client.

### MonsterAIComponent

High-level behavior state and shooting parameters for monsters. Movement patterns are defined by the shared `MovementComponent` (see `shared/include/components/MovementComponent.hpp`).

```cpp
#include <cstdint>

using EntityId = std::uint32_t; // must match ecs/Registry.hpp

struct MonsterAIComponent {
    // Shooting parameters
    float shootInterval = 0.0F; // seconds between shots (0 = no shooting)
    float shootCooldown = 0.0F; // seconds until next shot

    // Optional target tracking
    bool hasTarget = false;
    EntityId target = 0;       // valid only if hasTarget == true
};
```

- Invariants: `shootInterval >= 0`, `shootCooldown >= 0`.
- Systems: MonsterSystem (reads/writes cooldown, fires missiles), SpawnSystem (can initialize values).
- Networking: not replicated; it is server-internal state.

### MissileComponent

Data for projectiles fired by players or monsters.

```cpp
#include <cstdint>

using EntityId = std::uint32_t; // must match ecs/Registry.hpp

struct MissileComponent {
    EntityId owner = 0;       // entity that fired this missile
    bool fromPlayer = false;  // true if fired by player; affects collision masks

    std::int32_t damage = 1;  // damage dealt on hit

    // Kinematics
    float speed = 0.0F;       // units/sec along (dirX, dirY)
    float dirX = 1.0F;        // normalized X direction
    float dirY = 0.0F;        // normalized Y direction

    // Lifetime
    float lifetime = 2.0F;    // seconds before auto-despawn
};
```

- Invariants: `damage >= 0`, `speed >= 0`, `lifetime >= 0`, `(dirX, dirY)` should be normalized.
- Systems: MissileSystem (moves and decrements lifetime), CollisionSystem (applies damage), DestructionSystem (removes when lifetime <= 0).
- Networking: replicated minimally (position via Transform, and spawn/despawn events).

---

## Shared vs Server-Only Placement

- Shared (recommended): `TransformComponent`, `VelocityComponent`, `HitboxComponent`, `HealthComponent`, `MissileComponent`, `MovementComponent` (already exists), `PlayerInputComponent` (for reconciliation UI/client prediction if needed).
- Server-only (initially): `MonsterAIComponent` (purely server logic). If the client needs to render shooting telegraphs, consider a read-only mirror later.

Proposed headers (not implemented here):
- `shared/include/components/TransformComponent.hpp`
- `shared/include/components/VelocityComponent.hpp`
- `shared/include/components/HitboxComponent.hpp`
- `shared/include/components/HealthComponent.hpp`
- `shared/include/components/PlayerInputComponent.hpp`
- `shared/include/components/MonsterAIComponent.hpp` (could also be server-only)
- `shared/include/components/MissileComponent.hpp`

All re-exported by `shared/include/components/Components.hpp`.

---

## Example Usage With Registry

Create a player entity with transform, velocity, hitbox, health, and input:

```cpp
#include "ecs/Registry.hpp"
#include "components/Components.hpp"

Registry registry;
EntityId player = registry.createEntity();

registry.emplace<TransformComponent>(player, TransformComponent{100.0F, 200.0F, 0.0F});
registry.emplace<VelocityComponent>(player, VelocityComponent{0.0F, 0.0F});
registry.emplace<HitboxComponent>(player, HitboxComponent{32.0F, 16.0F});
registry.emplace<HealthComponent>(player, HealthComponent{3, 3, false});
registry.emplace<PlayerInputComponent>(player, PlayerInputComponent{});
```

Advance positions in a MovementSystem:

```cpp
for (EntityId id : registry.view<TransformComponent, VelocityComponent>()) {
    auto &t = registry.get<TransformComponent>(id);
    auto &v = registry.get<VelocityComponent>(id);
    t.x += v.vx * dt;
    t.y += v.vy * dt; // y grows downward (screen space)
}
```

Spawn a missile from a player:

```cpp
EntityId m = registry.createEntity();
registry.emplace<TransformComponent>(m, TransformComponent{t.x, t.y, t.rotation});
registry.emplace<HitboxComponent>(m, HitboxComponent{8.0F, 4.0F, 0.0F, 0.0F, true});
registry.emplace<MissileComponent>(m, MissileComponent{player, true, 1, 300.0F, 1.0F, 0.0F, 2.0F});
registry.emplace<VelocityComponent>(m, VelocityComponent{300.0F, 0.0F});
```

---

## Networking and Serialization Notes

- Replicate `TransformComponent` (x, y, rotation) on spawn and when changed (delta updates).
- Replicate `HealthComponent.current` on change; include `max` on spawn.
- Do not replicate `MonsterAIComponent` or internal cooldowns; they are server-only.
- `PlayerInputComponent` is inbound-only from the owning client; never broadcast.
- `MissileComponent` data typically does not need replication; clients derive visuals from transform and spawn/despawn events.

Binary packing order for common payloads should follow the natural struct field order and use fixed-size types where applicable to avoid padding issues across platforms. For floats, IEEE 754 32-bit is assumed.

---

## Interactions and Responsible Systems

- MovementSystem: reads `Velocity`, writes `Transform`.
- CollisionSystem: reads `Transform`, `Hitbox`, emits damage events.
- PlayerSystem: reads `PlayerInput`, writes `Transform/Velocity/rotation` as allowed.
- MonsterSystem: reads `MovementComponent`, `MonsterAI`, spawns missiles, updates cooldowns.
- MissileSystem: reads `Missile`, moves missiles (via `Velocity` or direct transform), decrements lifetime.
- DestructionSystem: removes entities when `Health.current <= 0` or `Missile.lifetime <= 0`.

---

## Validation Rules and Defaults

- Keep component constructors trivial; prefer aggregate initialization.
- Normalize directions `(dirX, dirY)` where required.
- Clamp `Health.current` to `[0, max]` when applying damage/heal.
- Use center-based transforms for all collision and rendering computations.
- Treat AABB as unrotated even if `Transform.rotation` is used for visuals.

---

## FAQ / Decisions

- Why y-down? Matches screen space for simpler client rendering and UI math.
- Why center origin? Simplifies symmetric sprites, rotations, and AABB offsets.
- Why keep rotation now? Needed for aiming/ship orientation and visuals; physics remains AABB-based for MVP.
- Where is movement pattern stored? In the shared `MovementComponent` for reuse; `MonsterAIComponent` only covers shooting/targeting state.
