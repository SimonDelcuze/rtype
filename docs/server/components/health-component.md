# HealthComponent

**Location:** `shared/include/components/HealthComponent.hpp` (planned)

The `HealthComponent` manages the health pool for damageable entities.

---

## **Structure**

```cpp
struct HealthComponent {
    std::int32_t current = 1;      // Current HP
    std::int32_t max = 1;          // Maximum HP
    bool invulnerable = false;     // If true, ignores incoming damage
};
```

---

## **Fields**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `current` | `int32_t` | `1` | Current health points |
| `max` | `int32_t` | `1` | Maximum health points |
| `invulnerable` | `bool` | `false` | If true, entity cannot take damage |

---

## **Invariants**

* `0 <= current <= max`
* `max >= 0`

---

## **Usage**

### Creating Entity with Health

```cpp
registry.emplace<HealthComponent>(player, HealthComponent{
    100,   // current HP
    100,   // max HP
    false  // not invulnerable
});
```

### Applying Damage

```cpp
auto& health = registry.get<HealthComponent>(enemy);

if (!health.invulnerable) {
    health.current -= damage;
    health.current = std::max(0, health.current);  // Clamp to 0
}

if (health.current <= 0) {
    registry.destroyEntity(enemy);
}
```

### Healing

```cpp
auto& health = registry.get<HealthComponent>(player);

health.current += healAmount;
health.current = std::min(health.current, health.max);  // Clamp to max
```

---

## **Systems**

* **CollisionSystem** — Applies damage from hits
* **DestructionSystem** — Removes entities when `current <= 0`
* **UI/HUD** — Displays health bars

---

## **Networking**

* **Replicate `current`** on change
* **Replicate `max`** on spawn
* **Don't replicate `invulnerable`** (server-only state)

---

## **Examples**

### Boss with High HP

```cpp
registry.emplace<HealthComponent>(boss, HealthComponent{5000, 5000, false});
```

### Invulnerable Player (during respawn)

```cpp
auto& health = registry.get<HealthComponent>(player);
health.invulnerable = true;  // Temporary invincibility

// After 3 seconds
health.invulnerable = false;
```

---

## **Related**

* [HitboxComponent](hitbox-component.md)
* [MissileComponent](missile-component.md)
