# ScoreComponent

**Location:** `shared/include/components/ScoreComponent.hpp`

Tracks the player's score as an integer and provides helpers to mutate and query it safely.

---

## Structure

```cpp
struct ScoreComponent
{
    int value = 0;

    static ScoreComponent create(int initial);

    void add(int amount);
    void subtract(int amount);
    void reset();
    void set(int newValue);
    bool isZero() const;
    bool isPositive() const;
};
```

---

## Behavior

- `value` is always non-negative after `subtract`/`set` clamping.
- `add`/`subtract` ignore non-positive amounts to avoid accidental sign errors.
- `reset` zeroes the score.
- `isZero`/`isPositive` are convenience checks for HUD logic or achievements.

---

## Usage

```cpp
// Create with an initial score
auto score = ScoreComponent::create(500);
registry.emplace<ScoreComponent>(playerId, score);

// Increase score (e.g., on kill)
auto& sc = registry.get<ScoreComponent>(playerId);
sc.add(100);

// Clamp to zero on penalty
sc.subtract(300);
```

---

## Notes

- Component is shared-side, safe to replicate; client HUD reads it to render "Score: X".
- Business rules (combo multipliers, per-event scoring) live outside; keep this component as pure state.
