# VelocityComponent

**Location:** `shared/include/components/VelocityComponent.hpp` (planned)

The `VelocityComponent` represents an entity's linear velocity in the game world. It is used by the `MovementSystem` to update the entity's `TransformComponent` based on delta time.

---

## **Structure**

```cpp
struct VelocityComponent {
    float vx = 0.0F;  // Velocity on X-axis (units/second)
    float vy = 0.0F;  // Velocity on Y-axis (units/second)
};
```

---

## **Fields**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `vx` | `float` | `0.0F` | Horizontal velocity in units per second (positive = right) |
| `vy` | `float` | `0.0F` | Vertical velocity in units per second (positive = down) |

---

## **Invariants**

* **Finite values** — `vx` and `vy` must be finite (not NaN or infinite)
* **Units** — Measured in world units per second
* **Direction** — Positive vx = right, positive vy = down (screen space)

---

## **Usage**

### Creating a Moving Entity

```cpp
Registry registry;
EntityId bullet = registry.createEntity();

registry.emplace<TransformComponent>(bullet, TransformComponent{100.0F, 200.0F, 0.0F});
registry.emplace<VelocityComponent>(bullet, VelocityComponent{300.0F, 0.0F});  // 300 units/s to the right
```

### Changing Velocity

```cpp
auto& velocity = registry.get<VelocityComponent>(player);

// Accelerate right
velocity.vx += 50.0F;

// Stop vertical movement
velocity.vy = 0.0F;
```

### Setting Velocity from Direction and Speed

```cpp
auto& velocity = registry.get<VelocityComponent>(missile);

float speed = 400.0F;  // units/second
float angle = M_PI / 4.0F;  // 45 degrees

velocity.vx = speed * std::cos(angle);
velocity.vy = speed * std::sin(angle);
```

---

## **Integration with MovementSystem**

The `MovementSystem` integrates velocity into position using **Euler integration**:

```cpp
void MovementSystem::update(Registry& registry, float deltaTime) {
    for (EntityId id : registry.view<TransformComponent, VelocityComponent>()) {
        auto& transform = registry.get<TransformComponent>(id);
        auto& velocity = registry.get<VelocityComponent>(id);
        
        // Integrate position
        transform.x += velocity.vx * deltaTime;
        transform.y += velocity.vy * deltaTime;
    }
}
```

**Example:**
* Velocity: `(200, 0)` units/s
* Delta time: `0.016s` (60 FPS)
* Position change: `(3.2, 0)` units

---

## **Systems That Use Velocity**

### MovementSystem (Primary Consumer)

Updates position based on velocity and delta time.

### PlayerSystem (Writer)

Sets velocity based on player input:

```cpp
auto& velocity = registry.get<VelocityComponent>(playerId);
auto& input = registry.get<PlayerInputComponent>(playerId);

constexpr float PLAYER_SPEED = 200.0F;

velocity.vx = input.moveX * PLAYER_SPEED;
velocity.vy = input.moveY * PLAYER_SPEED;
```

### MissileSystem (Writer)

Maintains constant velocity for projectiles:

```cpp
auto& velocity = registry.get<VelocityComponent>(missileId);
auto& missile = registry.get<MissileComponent>(missileId);

velocity.vx = missile.speed * missile.dirX;
velocity.vy = missile.speed * missile.dirY;
```

### MonsterSystem (Writer)

Applies AI movement patterns via `MovementComponent` (which may modify velocity).

---

## **Examples**

### Stationary Entity

```cpp
// Entity at rest
registry.emplace<VelocityComponent>(turret, VelocityComponent{0.0F, 0.0F});
```

### Constant Velocity

```cpp
// Enemy moving left at 100 units/s
registry.emplace<VelocityComponent>(enemy, VelocityComponent{-100.0F, 0.0F});
```

### Diagonal Movement

```cpp
// Missile moving at 45° downward-right at 300 units/s
float speed = 300.0F;
float angle = M_PI / 4.0F;  // 45 degrees

registry.emplace<VelocityComponent>(missile, VelocityComponent{
    speed * std::cos(angle),  // vx = 212.1
    speed * std::sin(angle)   // vy = 212.1
});
```

### Player WASD Movement

```cpp
constexpr float PLAYER_SPEED = 250.0F;

// Input: W key pressed
velocity.vx = 0.0F;
velocity.vy = -PLAYER_SPEED;  // Move up (negative Y)

// Input: D key pressed
velocity.vx = PLAYER_SPEED;   // Move right
velocity.vy = 0.0F;

// Input: W + D keys pressed (diagonal)
velocity.vx = PLAYER_SPEED / std::sqrt(2.0F);  // Normalize to maintain speed
velocity.vy = -PLAYER_SPEED / std::sqrt(2.0F);
```

---

## **Velocity Patterns**

### Stopping Gradually (Friction/Drag)

```cpp
constexpr float DRAG = 0.95F;  // 5% speed loss per frame

for (EntityId id : registry.view<VelocityComponent>()) {
    auto& velocity = registry.get<VelocityComponent>(id);
    
    velocity.vx *= DRAG;
    velocity.vy *= DRAG;
    
    // Stop completely when very slow
    if (std::abs(velocity.vx) < 0.1F) velocity.vx = 0.0F;
    if (std::abs(velocity.vy) < 0.1F) velocity.vy = 0.0F;
}
```

### Clamping Max Speed

```cpp
constexpr float MAX_SPEED = 500.0F;

auto& velocity = registry.get<VelocityComponent>(entity);

float speed = std::sqrt(velocity.vx * velocity.vx + velocity.vy * velocity.vy);

if (speed > MAX_SPEED) {
    velocity.vx = (velocity.vx / speed) * MAX_SPEED;
    velocity.vy = (velocity.vy / speed) * MAX_SPEED;
}
```

### Bouncing Off Walls

```cpp
// Hit left/right wall
if (transform.x <= 0 || transform.x >= WORLD_WIDTH) {
    velocity.vx = -velocity.vx;  // Reverse horizontal direction
}

// Hit top/bottom wall
if (transform.y <= 0 || transform.y >= WORLD_HEIGHT) {
    velocity.vy = -velocity.vy;  // Reverse vertical direction
}
```

---

## **Networking**

### Replication

* **Not replicated by default** — Clients can infer velocity from position deltas
* **Optional** — Can be sent for smoother client-side prediction

### Client-Side Prediction

Client can store velocity locally:

```cpp
// Client receives position update
clientTransform.x = serverTransform.x;
clientTransform.y = serverTransform.y;

// Estimate velocity from delta
float dt = timeSinceLastUpdate;
clientVelocity.vx = (serverTransform.x - lastTransform.x) / dt;
clientVelocity.vy = (serverTransform.y - lastTransform.y) / dt;
```

---

## **Performance Considerations**

* **Tiny memory** — 8 bytes per component
* **Cache-friendly** — Simple POD struct
* **No allocations** — Stack-based operations

---

## **Validation**

```cpp
bool isValid(const VelocityComponent& v) {
    return std::isfinite(v.vx) && std::isfinite(v.vy);
}
```

---

## **Related Components**

* [TransformComponent](transform-component.md) — Position updated by velocity
* [MissileComponent](missile-component.md) — Stores direction and speed
* [MovementComponent](movement-component.md) — AI patterns that affect velocity

---

## **Related Systems**

* **MovementSystem** — Integrates velocity into position
* **PlayerSystem** — Sets velocity from input
* **MissileSystem** — Maintains constant missile velocity
* **MonsterSystem** — Applies AI movement patterns
