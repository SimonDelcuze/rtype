# ColliderComponent

## Overview

The `ColliderComponent` is a critical physics and gameplay component in the R-Type client that defines the collision shape and properties for entities that need to interact with each other through collisions. While the authoritative collision detection happens on the server, the client uses `ColliderComponent` for visual debug rendering, client-side prediction, and local interaction with non-networked entities.

## Purpose and Design Philosophy

Collision detection is fundamental to shoot'em-up gameplay:

1. **Hit Detection**: Determining when projectiles hit enemies or players
2. **Obstacle Interaction**: Preventing players from passing through walls
3. **Pickup Collection**: Detecting when players collect power-ups
4. **Debug Visualization**: Rendering hitboxes for development and debugging
5. **Client Prediction**: Enabling responsive local collision feedback

The `ColliderComponent` supports multiple collision shape types to efficiently represent different entity geometries while maintaining good performance.

## Component Structure

```cpp
struct ColliderComponent
{
    enum class Shape
    {
        Box,
        Circle,
        Polygon
    };

    Shape shape   = Shape::Box;
    float offsetX = 0.0F;
    float offsetY = 0.0F;
    float width   = 0.0F;
    float height  = 0.0F;
    float radius  = 0.0F;
    bool isActive = true;
    std::vector<std::array<float, 2>> points;

    static ColliderComponent box(float width, float height, 
                                  float offsetX = 0.0F, float offsetY = 0.0F,
                                  bool active = true);
    static ColliderComponent circle(float radius, 
                                     float offsetX = 0.0F, float offsetY = 0.0F,
                                     bool active = true);
    static ColliderComponent polygon(const std::vector<std::array<float, 2>>& pts,
                                      float offsetX = 0.0F, float offsetY = 0.0F,
                                      bool active = true);
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `shape` | `Shape` | `Box` | The collision shape type: Box, Circle, or Polygon. |
| `offsetX` | `float` | `0.0F` | Horizontal offset from entity origin to collider center. |
| `offsetY` | `float` | `0.0F` | Vertical offset from entity origin to collider center. |
| `width` | `float` | `0.0F` | Width of box collider (only for Box shape). |
| `height` | `float` | `0.0F` | Height of box collider (only for Box shape). |
| `radius` | `float` | `0.0F` | Radius of circle collider (only for Circle shape). |
| `isActive` | `bool` | `true` | Whether this collider is currently enabled for collision detection. |
| `points` | `vector<array<float,2>>` | `{}` | Vertices for polygon colliders (only for Polygon shape). |

### Shape Enumeration

```cpp
enum class Shape
{
    Box,      // Axis-aligned bounding box
    Circle,   // Circular collider
    Polygon   // Convex polygon collider
};
```

## Factory Methods

### Box Collider

Creates an axis-aligned rectangular collider.

```cpp
static ColliderComponent box(float width, float height, 
                              float offsetX = 0.0F, float offsetY = 0.0F,
                              bool active = true);
```

**Example:**
```cpp
// Create a 32x32 box collider centered on entity
auto collider = ColliderComponent::box(32.0F, 32.0F);

// Create a box collider with offset (e.g., for sprite alignment)
auto offsetCollider = ColliderComponent::box(64.0F, 48.0F, -32.0F, -24.0F);
```

### Circle Collider

Creates a circular collider, ideal for round objects or simplified collision.

```cpp
static ColliderComponent circle(float radius, 
                                 float offsetX = 0.0F, float offsetY = 0.0F,
                                 bool active = true);
```

**Example:**
```cpp
// Create a circular collider with radius 16
auto circleCollider = ColliderComponent::circle(16.0F);

// Create an offset circular collider
auto bulletCollider = ColliderComponent::circle(8.0F, 4.0F, 0.0F);
```

### Polygon Collider

Creates a convex polygon collider for complex shapes.

```cpp
static ColliderComponent polygon(const std::vector<std::array<float, 2>>& pts,
                                  float offsetX = 0.0F, float offsetY = 0.0F,
                                  bool active = true);
```

**Example:**
```cpp
// Create a triangle collider
std::vector<std::array<float, 2>> trianglePoints = {
    {0.0F, -20.0F},   // Top
    {-15.0F, 10.0F},  // Bottom left
    {15.0F, 10.0F}    // Bottom right
};
auto triangleCollider = ColliderComponent::polygon(trianglePoints);
```

## Usage Patterns

### Player Ship Collider

The player ship typically uses a small box or circle for fair hit detection:

```cpp
Entity createPlayerShip(Registry& registry, float x, float y)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    
    // Player sprite is 64x32, but hitbox is smaller for fairness
    registry.addComponent(entity, SpriteComponent::create("player_ship", 64, 32));
    
    // Small centered hitbox (16x8) - makes the game more forgiving
    registry.addComponent(entity, ColliderComponent::box(16.0F, 8.0F, -8.0F, -4.0F));
    
    registry.addComponent(entity, LayerComponent::create(RenderLayer::Entities));
    
    return entity;
}
```

### Enemy with Complex Hitbox

Large enemies may need polygon colliders:

```cpp
Entity createBoss(Registry& registry, float x, float y)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    registry.addComponent(entity, SpriteComponent::create("boss_sprite", 256, 192));
    registry.addComponent(entity, BossComponent::create("Gomander"));
    
    // Complex shape to match boss visual
    std::vector<std::array<float, 2>> bossShape = {
        {-100.0F, -50.0F},
        {-60.0F, -80.0F},
        {60.0F, -80.0F},
        {100.0F, -50.0F},
        {100.0F, 50.0F},
        {-100.0F, 50.0F}
    };
    registry.addComponent(entity, ColliderComponent::polygon(bossShape));
    
    return entity;
}
```

### Projectile Collider

Projectiles typically use small circles for efficient collision:

```cpp
Entity spawnProjectile(Registry& registry, float x, float y, float vx, float vy)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    registry.addComponent(entity, VelocityComponent::create(vx, vy));
    registry.addComponent(entity, SpriteComponent::create("bullet", 16, 8));
    
    // Small circular collider for projectile
    registry.addComponent(entity, ColliderComponent::circle(4.0F));
    
    return entity;
}
```

### Power-up Pickup Area

Power-ups may have larger collision areas for easier collection:

```cpp
Entity spawnPowerUp(Registry& registry, float x, float y, PowerUpType type)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    registry.addComponent(entity, SpriteComponent::create("powerup_" + toString(type)));
    
    // Larger pickup radius for easier collection
    registry.addComponent(entity, ColliderComponent::circle(24.0F));
    
    registry.addComponent(entity, PowerUpComponent{type});
    
    return entity;
}
```

## Debug Visualization

The `HitboxDebugSystem` renders colliders for debugging:

```cpp
void HitboxDebugSystem::render(Registry& registry, Window& window)
{
    if (!m_debugEnabled) return;
    
    auto view = registry.view<ColliderComponent, TransformComponent>();
    
    for (auto entity : view) {
        auto& collider = view.get<ColliderComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        
        if (!collider.isActive) continue;
        
        Color debugColor = collider.isActive ? Color{255, 0, 0, 128} : Color{128, 128, 128, 64};
        
        switch (collider.shape) {
            case ColliderComponent::Shape::Box:
                drawDebugBox(window, 
                    transform.x + collider.offsetX,
                    transform.y + collider.offsetY,
                    collider.width, collider.height,
                    debugColor);
                break;
                
            case ColliderComponent::Shape::Circle:
                drawDebugCircle(window,
                    transform.x + collider.offsetX,
                    transform.y + collider.offsetY,
                    collider.radius,
                    debugColor);
                break;
                
            case ColliderComponent::Shape::Polygon:
                drawDebugPolygon(window,
                    transform.x + collider.offsetX,
                    transform.y + collider.offsetY,
                    collider.points,
                    debugColor);
                break;
        }
    }
}
```

## Collision Detection Algorithms

### Box vs Box (AABB)

```cpp
bool checkBoxCollision(const ColliderComponent& a, const TransformComponent& ta,
                       const ColliderComponent& b, const TransformComponent& tb)
{
    float ax = ta.x + a.offsetX;
    float ay = ta.y + a.offsetY;
    float bx = tb.x + b.offsetX;
    float by = tb.y + b.offsetY;
    
    return (ax < bx + b.width &&
            ax + a.width > bx &&
            ay < by + b.height &&
            ay + a.height > by);
}
```

### Circle vs Circle

```cpp
bool checkCircleCollision(const ColliderComponent& a, const TransformComponent& ta,
                          const ColliderComponent& b, const TransformComponent& tb)
{
    float dx = (tb.x + b.offsetX) - (ta.x + a.offsetX);
    float dy = (tb.y + b.offsetY) - (ta.y + a.offsetY);
    float distanceSq = dx * dx + dy * dy;
    float radiusSum = a.radius + b.radius;
    
    return distanceSq < radiusSum * radiusSum;
}
```

### Box vs Circle

```cpp
bool checkBoxCircleCollision(const ColliderComponent& box, const TransformComponent& tBox,
                              const ColliderComponent& circle, const TransformComponent& tCircle)
{
    float circleX = tCircle.x + circle.offsetX;
    float circleY = tCircle.y + circle.offsetY;
    float boxX = tBox.x + box.offsetX;
    float boxY = tBox.y + box.offsetY;
    
    // Find closest point on box to circle center
    float closestX = std::max(boxX, std::min(circleX, boxX + box.width));
    float closestY = std::max(boxY, std::min(circleY, boxY + box.height));
    
    float dx = circleX - closestX;
    float dy = circleY - closestY;
    
    return (dx * dx + dy * dy) < (circle.radius * circle.radius);
}
```

## Active State Management

Colliders can be temporarily disabled:

```cpp
// Disable collision during invincibility frames
void setInvincible(Registry& registry, Entity entity, bool invincible)
{
    if (registry.hasComponent<ColliderComponent>(entity)) {
        auto& collider = registry.getComponent<ColliderComponent>(entity);
        collider.isActive = !invincible;
    }
}
```

## Best Practices

1. **Hitbox Size**: Make player hitboxes smaller than sprites for fairness
2. **Simple Shapes**: Prefer circles and boxes over polygons for performance
3. **Offset Alignment**: Use offsets to center colliders on sprite visuals
4. **Active State**: Disable colliders during invincibility or transitions
5. **Shape Selection**: Use circles for round objects, boxes for rectangular

## Performance Considerations

| Shape | Collision Check Cost | Memory | Best For |
|-------|---------------------|--------|----------|
| Box | O(1) | 16 bytes | Most entities |
| Circle | O(1) | 4 bytes | Projectiles, pickups |
| Polygon | O(n) | Variable | Complex bosses |

## Related Components

- [TransformComponent](../server/components/transform-component.md) - Provides position
- [HitboxComponent](../server/components/hitbox-component.md) - Server-side equivalent
- [VelocityComponent](../server/components/velocity-component.md) - For swept collision

## Related Systems

- [HitboxDebugSystem](../systems/hitbox-debug-system.md) - Debug visualization
- [CollisionSystem](../server/collision-system.md) - Server collision processing
