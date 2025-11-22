# MissileComponent

**Location:** `shared/include/components/MissileComponent.hpp` (planned)

The `MissileComponent` contains data for projectiles fired by players or monsters.

---

## **Structure**

```cpp
struct MissileComponent {
    EntityId owner = 0;           // Entity that fired this missile
    bool fromPlayer = false;      // True if fired by player (affects collision masks)
    std::int32_t damage = 1;      // Damage dealt on hit
    float speed = 0.0F;           // Units/second along (dirX, dirY)
    float dirX = 1.0F;            // Normalized X direction
    float dirY = 0.0F;            // Normalized Y direction
    float lifetime = 2.0F;        // Seconds before auto-despawn
};
```

---

## **Fields**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `owner` | `EntityId` | `0` | Entity that fired the missile |
| `fromPlayer` | `bool` | `false` | If true, missile was fired by a player |
| `damage` | `int32_t` | `1` | Damage inflicted on hit |
| `speed` | `float` | `0.0F` | Movement speed in units/second |
| `dirX` | `float` | `1.0F` | Normalized X direction |
| `dirY` | `float` | `0.0F` | Normalized Y direction |
| `lifetime` | `float` | `2.0F` | Time until auto-destruction (seconds) |

---

## **Invariants**

* `damage >= 0`
* `speed >= 0`
* `lifetime >= 0`
* `(dirX, dirY)` should be normalized: `sqrt(dirX² + dirY²) ≈ 1.0`

---

## **Usage**

### Player Firing Missile

```cpp
EntityId missile = registry.createEntity();

auto& playerTrans = registry.get<TransformComponent>(player);

registry.emplace<TransformComponent>(missile, TransformComponent{
    playerTrans.x + 30.0F,  // Offset in front
    playerTrans.y,
    playerTrans.rotation
});

registry.emplace<MissileComponent>(missile, MissileComponent{
    player,    // owner
    true,      // fromPlayer
    10,        // damage
    400.0F,    // speed
    1.0F,      // dirX (right)
    0.0F,      // dirY
    3.0F       // lifetime
});

registry.emplace<VelocityComponent>(missile, VelocityComponent{400.0F, 0.0F});
registry.emplace<HitboxComponent>(missile, HitboxComponent{8.0F, 4.0F, 0.0F, 0.0F, true});
```

### Monster Firing Missile

```cpp
EntityId missile = registry.createEntity();

registry.emplace<MissileComponent>(missile, MissileComponent{
    monsterId,  // owner
    false,      // fromPlayer (monster missile)
    15,         // damage
    200.0F,     // speed
    -1.0F,      // dirX (left)
    0.0F,       // dirY
    5.0F        // lifetime
});
```

---

## **MissileSystem Logic**

### Movement

```cpp
for (EntityId id : registry.view<MissileComponent, VelocityComponent>()) {
    auto& missile = registry.get<MissileComponent>(id);
    auto& velocity = registry.get<VelocityComponent>(id);
    
    // Maintain constant velocity
    velocity.vx = missile.speed * missile.dirX;
    velocity.vy = missile.speed * missile.dirY;
}
```

### Lifetime Management

```cpp
for (EntityId id : registry.view<MissileComponent>()) {
    auto& missile = registry.get<MissileComponent>(id);
    
    missile.lifetime -= deltaTime;
    
    if (missile.lifetime <= 0.0F) {
        registry.destroyEntity(id);  // Timeout, destroy missile
    }
}
```

### Collision Handling

```cpp
// Missile hits enemy
if (checkCollision(missile, enemy)) {
    auto& missileComp = registry.get<MissileComponent>(missileId);
    auto& health = registry.get<HealthComponent>(enemyId);
    
    if (missileComp.fromPlayer) {  // Player missile hits enemy
        health.current -= missileComp.damage;
        registry.destroyEntity(missileId);  // Destroy missile on hit
    }
}
```

---

## **Collision Masks**

* **Player missiles** (`fromPlayer = true`) hit **monsters**
* **Monster missiles** (`fromPlayer = false`) hit **players**
* Missiles ignore entities with same team

---

## **Examples**

### Homing Missile (Advanced)

```cpp
// Update direction to track target
auto& missile = registry.get<MissileComponent>(missileId);
auto& missileTrans = registry.get<TransformComponent>(missileId);
auto& targetTrans = registry.get<TransformComponent>(targetId);

float dx = targetTrans.x - missileTrans.x;
float dy = targetTrans.y - missileTrans.y;
float dist = std::sqrt(dx * dx + dy * dy);

// Normalize direction
missile.dirX = dx / dist;
missile.dirY = dy / dist;
```

---

## **Systems**

* **MissileSystem** — Moves missiles, decrements lifetime
* **CollisionSystem** — Detects hits, applies damage
* **DestructionSystem** — Removes missiles when `lifetime <= 0`

---

## **Networking**

* **Minimal replication** — Spawn/despawn events only
* **Position via Transform** — Standard transform replication
* **Client prediction** — Clients can simulate missile movement locally

---

## **Related**

* [TransformComponent](transform-component.md)
* [Velocity Component](velocity-component.md)
* [HitboxComponent](hitbox-component.md)
* [HealthComponent](health-component.md)
* [MonsterAIComponent](monster-ai-component.md)
