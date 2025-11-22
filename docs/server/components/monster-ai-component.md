# MonsterAIComponent

**Location:** `shared/include/components/MonsterAIComponent.hpp` (planned)

The `MonsterAIComponent` manages high-level AI behavior for monsters, particularly shooting and targeting. Movement patterns are handled by [MovementComponent](movement-component.md).

---

## **Structure**

```cpp
struct MonsterAIComponent {
    float shootInterval = 0.0F;    // Seconds between shots (0 = no shooting)
    float shootCooldown = 0.0F;    // Seconds until next shot
    bool hasTarget = false;        // Whether monster is tracking a target
    EntityId target = 0;           // Target entity ID (valid only if hasTarget == true)
};
```

---

## **Fields**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `shootInterval` | `float` | `0.0F` | Time between shots (0 = doesn't shoot) |
| `shootCooldown` | `float` | `0.0F` | Time remaining until next shot |
| `hasTarget` | `bool` | `false` | Whether targeting is active |
| `target` | `EntityId` | `0` | Target entity ID (only valid if `hasTarget` is true) |

---

## **Invariants**

* `shootInterval >= 0`
* `shootCooldown >= 0`
* If `hasTarget == false`, `target` is ignored

---

## **Usage**

### Creating Shooting Monster

```cpp
registry.emplace<MonsterAIComponent>(monster, MonsterAIComponent{
    2.0F,    // shootInterval: shoot every 2 seconds
    0.0F,    // shootCooldown: ready to shoot immediately
    false,   // hasTarget
    0        // target (unused)
});
```

### MonsterSystem Update

```cpp
void MonsterSystem::update(Registry& registry, float deltaTime) {
    for (EntityId id : registry.view<MonsterAIComponent, TransformComponent>()) {
        auto& ai = registry.get<MonsterAIComponent>(id);
        auto& transform = registry.get<TransformComponent>(id);
        
        // Update cooldown
        if (ai.shootCooldown > 0.0F) {
            ai.shootCooldown -= deltaTime;
        }
        
        // Shoot if ready
        if (ai.shootInterval > 0.0F && ai.shootCooldown <= 0.0F) {
            spawnMissile(registry, id, transform);
            ai.shootCooldown = ai.shootInterval;  // Reset cooldown
        }
    }
}
```

### Targeting Player

```cpp
// Find nearest player
EntityId nearestPlayer = findNearestPlayer(registry, monsterPos);

auto& ai = registry.get<MonsterAIComponent>(monster);
ai.hasTarget = true;
ai.target = nearestPlayer;
```

---

## **Shooting Logic**

```cpp
void spawnMissile(Registry& registry, EntityId shooter, const TransformComponent& shooterTrans) {
    EntityId missile = registry.createEntity();
    
    // Spawn missile in front of shooter
    registry.emplace<TransformComponent>(missile, TransformComponent{
        shooterTrans.x + 20.0F,  // Offset forward
        shooterTrans.y,
        shooterTrans.rotation
    });
    
    registry.emplace<MissileComponent>(missile, MissileComponent{
        shooter,      // owner
        false,        // fromPlayer = false (monster missile)
        10,           // damage
        200.0F,       // speed
        -1.0F,        // dirX (left)
        0.0F,         // dirY
        3.0F          // lifetime
    });
    
    registry.emplace<VelocityComponent>(missile, VelocityComponent{-200.0F, 0.0F});
}
```

---

## **Examples**

### Non-Shooting Monster

```cpp
// Movement only, no shooting
registry.emplace<MonsterAIComponent>(monster, MonsterAIComponent{
    0.0F,  // shootInterval = 0 (doesn't shoot)
    0.0F,
    false,
    0
});
```

### Rapid-Fire Monster

```cpp
registry.emplace<MonsterAIComponent>(monster, MonsterAIComponent{
    0.5F,  // Shoot every 0.5 seconds
    0.0F,
    false,
    0
});
```

### Boss with Targeting

```cpp
registry.emplace<MonsterAIComponent>(boss, MonsterAIComponent{
    1.0F,      // Shoot every second
    0.0F,
    true,      // hasTarget
    playerId   // target player
});
```

---

## **Systems**

* **MonsterSystem** — Updates cooldowns, spawns missiles
* **SpawnSystem** — Initializes AI parameters for new monsters

---

## **Networking**

* **Not replicated** — Server-only internal state
* **Missile spawn events** are replicated to clients

---

## **Related**

* [MovementComponent](movement-component.md) — Handles monster movement patterns
* [MissileComponent](missile-component.md) — Projectiles spawned by monsters
* [TransformComponent](transform-component.md) — Needed for shooting direction
