# Core Components API

Shared components used by both client and server for game logic and network synchronization.

## TransformComponent

Represents an entity's position, rotation, and scale in 2D space.

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `x` | `float` | 0.0 | X position |
| `y` | `float` | 0.0 | Y position |
| `rotation` | `float` | 0.0 | Rotation angle in degrees |
| `scaleX` | `float` | 1.0 | Horizontal scale |
| `scaleY` | `float` | 1.0 | Vertical scale |

### Methods

```cpp
static TransformComponent create(float x, float y, float rotation = 0.0F);
void translate(float dx, float dy);
void rotate(float angle);
void scale(float sx, float sy);
```

---

## VelocityComponent

Represents an entity's velocity vector.

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `vx` | `float` | 0.0 | Horizontal velocity |
| `vy` | `float` | 0.0 | Vertical velocity |

### Methods

```cpp
static VelocityComponent create(float vx, float vy);
```

---

## HitboxComponent

Axis-aligned bounding box (AABB) for collision detection.

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `width` | `float` | 0.0 | Hitbox width |
| `height` | `float` | 0.0 | Hitbox height |
| `offsetX` | `float` | 0.0 | Horizontal offset from entity position |
| `offsetY` | `float` | 0.0 | Vertical offset from entity position |
| `isActive` | `bool` | true | Whether collision is enabled |

### Methods

```cpp
static HitboxComponent create(float width, float height, float offsetX = 0.0F, float offsetY = 0.0F);

// Check if a point is inside the hitbox
bool contains(float px, float py, float entityX, float entityY) const;

// Check if two hitboxes overlap
bool intersects(const HitboxComponent& other, float x1, float y1, float x2, float y2) const;
```

### Offset Usage

The offset allows the hitbox to be smaller or positioned differently than the sprite:

```
Sprite 64x64 with centered 32x32 hitbox:

+------------------+
|                  |
|   +---------+    |
|   | HITBOX  |    |
|   +---------+    |
|                  |
+------------------+

offsetX = 16, offsetY = 16
width = 32, height = 32
```

---

## HealthComponent

Manages entity health points with damage and healing.

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `current` | `int32_t` | 100 | Current health |
| `max` | `int32_t` | 100 | Maximum health |

### Methods

```cpp
static HealthComponent create(int32_t maxHealth);
void damage(int32_t amount);    // Clamps to 0
void heal(int32_t amount);      // Clamps to max
bool isDead() const;            // Returns true if current <= 0
float getPercentage() const;    // Returns current/max (0.0 to 1.0)
```

---

## OwnershipComponent

Identifies entity ownership for multiplayer.

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `ownerId` | `uint32_t` | 0 | Player/client ID who owns this entity |
| `team` | `uint8_t` | 0 | Team number |

### Methods

```cpp
static OwnershipComponent create(uint32_t ownerId, uint8_t team = 0);
bool isSameTeam(const OwnershipComponent& other) const;
bool isSameOwner(const OwnershipComponent& other) const;
```

---

## TagComponent

Bitfield tags for entity categorization and filtering.

### EntityTag Enum

```cpp
enum class EntityTag : uint8_t {
    None       = 0,
    Player     = 1 << 0,
    Enemy      = 1 << 1,
    Projectile = 1 << 2,
    Pickup     = 1 << 3,
    Obstacle   = 1 << 4,
    Background = 1 << 5
};
```

### Methods

```cpp
static TagComponent create(EntityTag tags);
bool hasTag(EntityTag tag) const;
void addTag(EntityTag tag);
void removeTag(EntityTag tag);
```

### Usage

```cpp
auto tag = TagComponent::create(EntityTag::Player | EntityTag::Projectile);
if (tag.hasTag(EntityTag::Player)) { /* ... */ }
tag.removeTag(EntityTag::Player);
```

---

## Serialization

Binary serialization helpers for network transmission.

### Methods

```cpp
namespace Serialization {
    // Serialize component to buffer
    void serialize(std::vector<uint8_t>& buffer, const TransformComponent& t);
    void serialize(std::vector<uint8_t>& buffer, const VelocityComponent& v);
    void serialize(std::vector<uint8_t>& buffer, const HitboxComponent& h);
    void serialize(std::vector<uint8_t>& buffer, const HealthComponent& h);
    void serialize(std::vector<uint8_t>& buffer, const OwnershipComponent& o);
    void serialize(std::vector<uint8_t>& buffer, const TagComponent& t);

    // Deserialize from buffer
    TransformComponent deserializeTransform(const uint8_t* data, size_t& offset);
    VelocityComponent deserializeVelocity(const uint8_t* data, size_t& offset);
    HitboxComponent deserializeHitbox(const uint8_t* data, size_t& offset);
    HealthComponent deserializeHealth(const uint8_t* data, size_t& offset);
    OwnershipComponent deserializeOwnership(const uint8_t* data, size_t& offset);
    TagComponent deserializeTag(const uint8_t* data, size_t& offset);
}
```

### Usage

```cpp
// Serialize
std::vector<uint8_t> buffer;
Serialization::serialize(buffer, transform);
Serialization::serialize(buffer, velocity);

// Send buffer over network...

// Deserialize
size_t offset = 0;
auto transform = Serialization::deserializeTransform(buffer.data(), offset);
auto velocity = Serialization::deserializeVelocity(buffer.data(), offset);
```

### Notes

- Uses raw binary serialization (fast, compact)
- Not portable across different architectures (endianness)
- No versioning support
- Suitable for same-architecture client/server communication

---

## Complete Example

```cpp
#include "components/Components.hpp"
#include "ecs/Registry.hpp"

Registry registry;

// Create player entity
EntityId player = registry.createEntity();

registry.emplace<TransformComponent>(player, TransformComponent::create(100.0F, 200.0F));
registry.emplace<VelocityComponent>(player, VelocityComponent::create(0.0F, 0.0F));
registry.emplace<HitboxComponent>(player, HitboxComponent::create(32.0F, 48.0F, 8.0F, 0.0F));
registry.emplace<HealthComponent>(player, HealthComponent::create(100));
registry.emplace<OwnershipComponent>(player, OwnershipComponent::create(1, 0));
registry.emplace<TagComponent>(player, TagComponent::create(EntityTag::Player));

// Create enemy entity
EntityId enemy = registry.createEntity();

registry.emplace<TransformComponent>(enemy, TransformComponent::create(400.0F, 200.0F));
registry.emplace<HitboxComponent>(enemy, HitboxComponent::create(32.0F, 32.0F));
registry.emplace<HealthComponent>(enemy, HealthComponent::create(50));
registry.emplace<TagComponent>(enemy, TagComponent::create(EntityTag::Enemy));

// Check collision
auto& playerTransform = registry.get<TransformComponent>(player);
auto& playerHitbox = registry.get<HitboxComponent>(player);
auto& enemyTransform = registry.get<TransformComponent>(enemy);
auto& enemyHitbox = registry.get<HitboxComponent>(enemy);

if (playerHitbox.intersects(enemyHitbox,
    playerTransform.x, playerTransform.y,
    enemyTransform.x, enemyTransform.y)) {
    registry.get<HealthComponent>(player).damage(10);
}
```
