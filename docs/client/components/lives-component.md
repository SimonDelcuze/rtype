# LivesComponent

**Location:** `shared/include/components/LivesComponent.hpp`

Represents the player's remaining lives/continues and maximum capacity. Provides helpers to add/remove lives with clamping.

---

## Structure

```cpp
struct LivesComponent
{
    int current = 0;
    int max     = 0;

    static LivesComponent create(int currentLives, int maxLives);

    void loseLife(int amount = 1);
    void addLife(int amount = 1);
    void addExtraLife(int amount = 1);
    void setMax(int newMax);
    void resetToMax();
    bool isDead() const;
};
```

---

## Behavior

- `loseLife` decrements and clamps to zero; ignores non-positive values.
- `addLife` increments up to `max`; ignores non-positive values.
- `addExtraLife` increases both `max` and `current` by the same positive amount.
- `setMax` clamps negative max to zero and trims `current` if above the new max.
- `resetToMax` sets `current = max`.
- `isDead` is true when `current <= 0`.

---

## Usage

```cpp
auto lives = LivesComponent::create(3, 3);
registry.emplace<LivesComponent>(playerId, lives);

// Lose one life on death
auto& lc = registry.get<LivesComponent>(playerId);
lc.loseLife();

// Extra life pickup
lc.addExtraLife();   // max and current +1

// Refill on respawn
lc.resetToMax();
```

---

## Notes

- Component is shared-side; replicated state can drive client HUD ("Lives: current/max").
- Distinct from `HealthComponent` (HP). Lives typically count respawns/continues, not hit points.
