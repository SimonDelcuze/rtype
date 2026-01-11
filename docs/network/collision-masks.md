# Collision Masks & Hitbox Alignment

Guide for aligning sprites with collision hitboxes using offsets and visual debug rendering.

## Overview

In R-Type, collision detection is **server-authoritative** (server calculates all collisions), but the client needs to:
1. **Align hitboxes** with sprites visually using offsets
2. **Debug render** hitboxes to verify alignment
3. **Receive authoritative** collision results from server

## HitboxComponent

Shared between client and server for consistent collision detection.

```cpp
#include "components/HitboxComponent.hpp"

struct HitboxComponent
{
    float width   = 0.0F;
    float height  = 0.0F;
    float offsetX = 0.0F;  // Offset from entity position
    float offsetY = 0.0F;
    bool isActive = true;

    static HitboxComponent create(float width, float height,
                                  float offsetX = 0.0F, float offsetY = 0.0F,
                                  bool active = true);

    bool contains(float px, float py, float entityX, float entityY) const;
    bool intersects(const HitboxComponent& other,
                   float x1, float y1, float x2, float y2) const;
};
```

## Hitbox Alignment

### Concept

Sprites often have transparent areas. The hitbox should match the visible part:

```
┌─────────────────┐  ← Sprite texture (64x64)
│    ░░░░░░░      │
│   ░██████░      │  ← Visible ship
│  ░████████░     │
│   ░██████░      │
│    ░░░░░░░      │
└─────────────────┘

Hitbox (30x40):
    ┌──────┐
    │      │  ← offsetX = 17, offsetY = 12
    │      │     aligns to ship center
    └──────┘
```

### Usage

```cpp
// Entity at (100, 100)
EntityId player = registry.createEntity();
auto& transform = registry.emplace<TransformComponent>(player);
transform.x = 100.0F;
transform.y = 100.0F;

// Sprite is 64x64
auto& sprite = registry.emplace<SpriteComponent>(player);
sprite.setTexture(*texture);

// Hitbox is 30x40, offset to center of sprite
auto& hitbox = registry.emplace<HitboxComponent>(player,
    HitboxComponent::create(
        30.0F,  // width
        40.0F,  // height
        17.0F,  // offsetX = (64 - 30) / 2
        12.0F   // offsetY = (64 - 40) / 2
    ));

// Actual hitbox position: (100 + 17, 100 + 12) = (117, 112)
```

## Calculating Offsets

### Centered Hitbox

To center a hitbox within a sprite:

```cpp
float spriteWidth = 64.0F;
float spriteHeight = 64.0F;
float hitboxWidth = 30.0F;
float hitboxHeight = 40.0F;

float offsetX = (spriteWidth - hitboxWidth) / 2.0F;   // (64 - 30) / 2 = 17
float offsetY = (spriteHeight - hitboxHeight) / 2.0F; // (64 - 40) / 2 = 12
```

### Custom Alignment

For asymmetric sprites (e.g., ship with cockpit at top):

```cpp
// Align hitbox to cockpit area at top of sprite
float offsetX = (spriteWidth - hitboxWidth) / 2.0F;  // Center horizontally
float offsetY = 5.0F;  // 5 pixels from top
```

## Visual Debug System

The `HitboxDebugSystem` renders hitbox outlines for debugging.

### HitboxDebugSystem

```cpp
#include "systems/HitboxDebugSystem.hpp"

class HitboxDebugSystem
{
public:
    explicit HitboxDebugSystem(Window& window);
    void update(Registry& registry);

    // Configuration
    void setEnabled(bool enabled);
    void setColor(const sf::Color& color);
    void setThickness(float thickness);
};
```

### Usage

```cpp
#include "systems/HitboxDebugSystem.hpp"

Window window(sf::VideoMode({800u, 600u}), "R-Type");
Registry registry;
HitboxDebugSystem hitboxDebug(window);

// Enable debug rendering
#ifdef DEBUG_MODE
hitboxDebug.setEnabled(true);
hitboxDebug.setColor(sf::Color::Green);
hitboxDebug.setThickness(2.0F);
#endif

// Game loop
while (running) {
    // ... other systems ...

    // Render entities
    renderSystem.update(registry);

    // Render hitboxes on top
    #ifdef DEBUG_MODE
    hitboxDebug.update(registry);
    #endif

    window.display();
}
```

## Complete Example

```cpp
#include "components/HitboxComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "systems/HitboxDebugSystem.hpp"

void createPlayer(Registry& registry, TextureManager& textures)
{
    EntityId player = registry.createEntity();

    // Position
    auto& transform = registry.emplace<TransformComponent>(player);
    transform.x = 100.0F;
    transform.y = 200.0F;

    // Sprite (64x64 texture)
    auto& sprite = registry.emplace<SpriteComponent>(player);
    sprite.setTexture(*textures.get("player_ship"));

    // Hitbox (smaller than sprite to match visible area)
    // Ship is 64x64 but visible area is ~40x50
    float spriteW = 64.0F;
    float spriteH = 64.0F;
    float hitboxW = 40.0F;
    float hitboxH = 50.0F;

    auto& hitbox = registry.emplace<HitboxComponent>(player,
        HitboxComponent::create(
            hitboxW,
            hitboxH,
            (spriteW - hitboxW) / 2.0F,  // offsetX = 12
            (spriteH - hitboxH) / 2.0F   // offsetY = 7
        ));
}

int main()
{
    Window window(sf::VideoMode({800u, 600u}), "R-Type");
    Registry registry;
    TextureManager textures;
    RenderSystem renderSystem(window);
    HitboxDebugSystem hitboxDebug(window);

    #ifdef DEBUG_MODE
    hitboxDebug.setEnabled(true);
    hitboxDebug.setColor(sf::Color::Green);
    #endif

    textures.load("player_ship", "assets/sprites/player.png");
    createPlayer(registry, textures);

    while (window.isOpen()) {
        window.clear();

        // Render sprites
        renderSystem.update(registry, deltaTime);

        // Render hitboxes
        #ifdef DEBUG_MODE
        hitboxDebug.update(registry);
        #endif

        window.display();
    }
}
```

## Server-Side Collision

The server uses the same `HitboxComponent` for authoritative collision detection:

```cpp
// Server collision system
void CollisionSystem::update(Registry& registry)
{
    auto projectiles = registry.view<HitboxComponent, TransformComponent, TagComponent>();
    auto enemies = registry.view<HitboxComponent, TransformComponent, TagComponent>();

    for (EntityId proj : projectiles) {
        auto& projTag = registry.get<TagComponent>(proj);
        if (!projTag.hasTag(EntityTag::Projectile)) continue;

        auto& projHitbox = registry.get<HitboxComponent>(proj);
        auto& projTransform = registry.get<TransformComponent>(proj);

        for (EntityId enemy : enemies) {
            auto& enemyTag = registry.get<TagComponent>(enemy);
            if (!enemyTag.hasTag(EntityTag::Enemy)) continue;

            auto& enemyHitbox = registry.get<HitboxComponent>(enemy);
            auto& enemyTransform = registry.get<TransformComponent>(enemy);

            if (projHitbox.intersects(enemyHitbox,
                projTransform.x, projTransform.y,
                enemyTransform.x, enemyTransform.y))
            {
                // Collision detected - send to clients
                handleCollision(proj, enemy);
            }
        }
    }
}
```

## Best Practices

### 1. Tight Hitboxes

Make hitboxes slightly smaller than sprites for fairer gameplay:

```cpp
// Sprite: 64x64
// Hitbox: 40x50 (about 80% of sprite size)
auto& hitbox = registry.emplace<HitboxComponent>(entity,
    HitboxComponent::create(40.0F, 50.0F, 12.0F, 7.0F));
```

### 2. Test with Debug Rendering

Always verify hitbox alignment visually:

```cpp
#ifdef DEBUG_MODE
hitboxDebug.setEnabled(true);
hitboxDebug.setColor(sf::Color(0, 255, 0, 128));  // Semi-transparent green
#endif
```

### 3. Consistent Offsets

Use helper functions for consistent alignment:

```cpp
struct HitboxHelper
{
    static HitboxComponent centered(float spriteW, float spriteH,
                                    float hitboxW, float hitboxH)
    {
        return HitboxComponent::create(
            hitboxW, hitboxH,
            (spriteW - hitboxW) / 2.0F,
            (spriteH - hitboxH) / 2.0F
        );
    }
};

// Usage
auto& hitbox = registry.emplace<HitboxComponent>(entity,
    HitboxHelper::centered(64.0F, 64.0F, 40.0F, 50.0F));
```

### 4. Dynamic Hitboxes

Adjust hitboxes for different states:

```cpp
// Normal state
hitbox.width = 40.0F;
hitbox.height = 50.0F;

// Powered up (bigger)
hitbox.width = 50.0F;
hitbox.height = 60.0F;

// Dodging (smaller)
hitbox.width = 30.0F;
hitbox.height = 40.0F;
```

### 5. Inactive Hitboxes

Disable hitboxes temporarily:

```cpp
// Make invincible
hitbox.isActive = false;

// Re-enable
hitbox.isActive = true;
```

## Troubleshooting

### Hitbox Not Visible

Check debug system is enabled:
```cpp
hitboxDebug.setEnabled(true);
```

### Hitbox Misaligned

Verify offsets:
```cpp
std::cout << "Entity pos: " << transform.x << ", " << transform.y << "\n";
std::cout << "Hitbox offset: " << hitbox.offsetX << ", " << hitbox.offsetY << "\n";
std::cout << "Actual hitbox pos: "
          << (transform.x + hitbox.offsetX) << ", "
          << (transform.y + hitbox.offsetY) << "\n";
```

### Collisions Feel Wrong

Adjust hitbox size:
```cpp
// Too sensitive? Make hitbox smaller
hitbox.width *= 0.9F;
hitbox.height *= 0.9F;

// Too forgiving? Make hitbox larger
hitbox.width *= 1.1F;
hitbox.height *= 1.1F;
```

## Debug Rendering Options

### Color Coding

Use different colors for different entity types:

```cpp
if (tag.hasTag(EntityTag::Player)) {
    hitboxDebug.setColor(sf::Color::Blue);
} else if (tag.hasTag(EntityTag::Enemy)) {
    hitboxDebug.setColor(sf::Color::Red);
} else if (tag.hasTag(EntityTag::Projectile)) {
    hitboxDebug.setColor(sf::Color::Yellow);
}
```

### Toggle with Keyboard

```cpp
if (sf::Keyboard::isKeyPressed(sf::Keyboard::F3)) {
    hitboxDebug.setEnabled(!hitboxDebug.isEnabled());
}
```

## Performance

- **HitboxDebugSystem**: O(n) where n = entities with hitboxes
- **Drawing**: One rectangle per entity per frame
- **Recommendation**: Disable in production builds with `#ifdef DEBUG_MODE`
