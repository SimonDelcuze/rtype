# MovementComponent

**Location:** `shared/include/components/MovementComponent.hpp`

The `MovementComponent` defines the movement behavior for entities in R-Type, particularly enemies. It supports multiple pattern types for varied gameplay.

---

## **Structure**

```cpp
struct MovementComponent {
    MovementPattern pattern = MovementPattern::Linear;
    float speed             = 0.0F;
    float amplitude         = 0.0F;
    float frequency         = 0.0F;
    float phase             = 0.0F;
    
    static MovementComponent linear(float speed);
    static MovementComponent zigzag(float speed, float amplitude, float frequency);
    static MovementComponent sine(float speed, float amplitude, float frequency, float phase = 0.0F);
};
```

---

## **Movement Patterns**

### `MovementPattern` Enum

```cpp
enum class MovementPattern : std::uint8_t {
    Linear,   // Straight-line movement
    Zigzag,   // Diagonal zigzag pattern
    Sine      // Smooth sinusoidal wave
};
```

---

## **Fields**

| Field | Type | Description |
|-------|------|-------------|
| `pattern` | `MovementPattern` | The movement pattern type |
| `speed` | `float` | Movement speed (pixels per second) |
| `amplitude` | `float` | Vertical amplitude for wave-based patterns |
| `frequency` | `float` | Oscillation frequency for wave patterns |
| `phase` | `float` | Initial phase offset for wave patterns |

---

## **Factory Methods**

### Linear Movement

```cpp
static MovementComponent linear(float speed);
```

Creates a component for straight-line movement.

**Parameters:**
* `speed` — Movement speed in pixels/second

**Example:**
```cpp
auto movement = MovementComponent::linear(150.0f);
registry.emplace<MovementComponent>(enemy, movement);
```

**Behavior:** Entity moves in a straight line (typically left for enemies).

---

### Zigzag Movement

```cpp
static MovementComponent zigzag(float speed, float amplitude, float frequency);
```

Creates a diagonal zigzag movement pattern.

**Parameters:**
* `speed` — Horizontal movement speed
* `amplitude` — Vertical height of each zig/zag
* `frequency` — How often the direction changes

**Example:**
```cpp
auto movement = MovementComponent::zigzag(100.0f, 50.0f, 2.0f);
registry.emplace<MovementComponent>(enemy, movement);
```

**Behavior:** Entity moves diagonally, reversing vertical direction periodically.

---

### Sine Wave Movement

```cpp
static MovementComponent sine(float speed, float amplitude, float frequency, float phase = 0.0F);
```

Creates a smooth sinusoidal wave movement.

**Parameters:**
* `speed` — Horizontal movement speed
* `amplitude` — Vertical amplitude of the wave
* `frequency` — Wave frequency (oscillations per second)
* `phase` — Initial phase offset (default: 0.0)

**Example:**
```cpp
auto movement = MovementComponent::sine(120.0f, 75.0f, 1.5f);
registry.emplace<MovementComponent>(enemy, movement);
```

**Behavior:** Entity follows a smooth wave pattern vertically while moving horizontally.

---

## **Usage in Systems**

The `MovementSystem` or `Mon sterSystem` processes entities with this component:

```cpp
// Example: MovementSystem update
for (EntityId id : registry.view<Position, MovementComponent>()) {
    auto& pos = registry.get<Position>(id);
    auto& move = registry.get<MovementComponent>(id);
    
    switch (move.pattern) {
        case MovementPattern::Linear:
            pos.x -= move.speed * deltaTime;
            break;
            
        case MovementPattern::Sine:
            pos.x -= move.speed * deltaTime;
            pos.y += move.amplitude * std::sin(2.0f * PI * move.frequency * time + move.phase);
            break;
            
        case MovementPattern::Zigzag:
            // Zigzag implementation
            break;
    }
}
```

---

## **Design Rationale**

### Why Factory Methods?

✅ **Clear intent** — `linear()`, `zigzag()`, `sine()` are self-documenting\
✅ **Safe defaults** — Unused fields are set to 0.0\
✅ **Consistency** — Prevents invalid configurations

### Why Multiple Patterns?

✅ **Gameplay variety** — Different enemy types have distinct behaviors\
✅ **Difficulty scaling** — Complex patterns increase challenge\
✅ **Classic R-Type feel** — Wave and zigzag patterns are iconic

---

## **Performance Considerations**

* **Lightweight** — Only 20 bytes per component
* **Cache-friendly** — Dense storage in sparse-set
* **Branch-predictable** — Pattern enum allows switch optimization

---

## **Testing**

See `tests/shared/ecs/MovementComponentTests.cpp`:

```cpp
TEST(MovementComponent, LinearFactorySetsSpeedOnly) {
    auto component = MovementComponent::linear(100.0F);
    EXPECT_EQ(component.pattern, MovementPattern::Linear);
    EXPECT_FLOAT_EQ(component.speed, 100.0F);
    EXPECT_FLOAT_EQ(component.amplitude, 0.0F);
}

TEST(MovementComponent, SineFactoryAppliesAllParameters) {
    auto component = MovementComponent::sine(50.0F, 10.0F, 2.0F, 1.5F);
    EXPECT_EQ(component.pattern, MovementPattern::Sine);
    EXPECT_FLOAT_EQ(component.speed, 50.0F);
    EXPECT_FLOAT_EQ(component.amplitude, 10.0F);
    EXPECT_FLOAT_EQ(component.frequency, 2.0F);
    EXPECT_FLOAT_EQ(component.phase, 1.5F);
}
```

---

## **Related**

* [Position Component](TODO) — Required for movement
* [Monster AI System](TODO) — Uses movement patterns
* [Movement System](TODO) — Processes this component
