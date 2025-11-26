# Scene Graph & Layering System

The layering system controls the render order of entities using the `LayerComponent` and `RenderSystem`.

## Overview

Entities are rendered from lowest to highest layer value, allowing you to create depth through:
- Background layers (parallax, skybox)
- Game entities (players, enemies, projectiles)
- Effects (particles, explosions)
- UI/HUD overlays

## LayerComponent

```cpp
#include "components/LayerComponent.hpp"

struct LayerComponent
{
    int layer = RenderLayer::Entities;  // Default layer

    static LayerComponent create(int layer);
};
```

## Standard Render Layers

Predefined constants for common layer types:

```cpp
namespace RenderLayer
{
    constexpr int Background = -100;  // Far background (parallax, skybox)
    constexpr int Midground  = -50;   // Middle background elements
    constexpr int Entities   = 0;     // Default layer for game entities
    constexpr int Effects    = 50;    // Particle effects, explosions
    constexpr int UI         = 100;   // UI elements
    constexpr int HUD        = 150;   // HUD overlay (health bars, scores)
    constexpr int Debug      = 200;   // Debug overlays, collision boxes
}
```

**Render Order**: Background → Midground → Entities → Effects → UI → HUD → Debug

## Usage

### Basic Layering

```cpp
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"

// Background entity
EntityId background = registry.createEntity();
registry.emplace<SpriteComponent>(background);
registry.emplace<TransformComponent>(background);
registry.emplace<LayerComponent>(background, LayerComponent::create(RenderLayer::Background));

// Player entity (default layer)
EntityId player = registry.createEntity();
registry.emplace<SpriteComponent>(player);
registry.emplace<TransformComponent>(player);
registry.emplace<LayerComponent>(player, LayerComponent::create(RenderLayer::Entities));

// HUD entity
EntityId healthBar = registry.createEntity();
registry.emplace<SpriteComponent>(healthBar);
registry.emplace<TransformComponent>(healthBar);
registry.emplace<LayerComponent>(healthBar, LayerComponent::create(RenderLayer::HUD));
```

### Custom Layers

You can use any integer value for custom layering:

```cpp
// Custom layers between standard ones
constexpr int LAYER_PLAYER = RenderLayer::Entities + 1;
constexpr int LAYER_ENEMY = RenderLayer::Entities + 2;

EntityId player = registry.createEntity();
// ... add components ...
registry.emplace<LayerComponent>(player, LayerComponent::create(LAYER_PLAYER));

EntityId enemy = registry.createEntity();
// ... add components ...
registry.emplace<LayerComponent>(enemy, LayerComponent::create(LAYER_ENEMY));

// Player renders first, then enemy
```

### Default Layer

If an entity has no `LayerComponent`, it defaults to `RenderLayer::Entities` (0):

```cpp
EntityId entity = registry.createEntity();
registry.emplace<SpriteComponent>(entity);
registry.emplace<TransformComponent>(entity);
// No LayerComponent - will render at layer 0 (Entities)
```

## Complete Example

```cpp
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/RenderSystem.hpp"

void setupGameScene(Registry& registry, TextureManager& textures)
{
    // 1. Background layer - scrolling space
    EntityId background = registry.createEntity();
    auto& bgSprite = registry.emplace<SpriteComponent>(background);
    bgSprite.setTexture(*textures.get("space_background"));
    registry.emplace<TransformComponent>(background);
    registry.emplace<LayerComponent>(background,
        LayerComponent::create(RenderLayer::Background));

    // 2. Entities layer - player
    EntityId player = registry.createEntity();
    auto& playerSprite = registry.emplace<SpriteComponent>(player);
    playerSprite.setTexture(*textures.get("player_ship"));
    registry.emplace<TransformComponent>(player);
    registry.emplace<LayerComponent>(player,
        LayerComponent::create(RenderLayer::Entities));

    // 3. Entities layer - enemies
    for (int i = 0; i < 5; ++i) {
        EntityId enemy = registry.createEntity();
        auto& enemySprite = registry.emplace<SpriteComponent>(enemy);
        enemySprite.setTexture(*textures.get("enemy_ship"));
        registry.emplace<TransformComponent>(enemy);
        registry.emplace<LayerComponent>(enemy,
            LayerComponent::create(RenderLayer::Entities));
    }

    // 4. Effects layer - particle systems
    EntityId explosion = registry.createEntity();
    auto& fxSprite = registry.emplace<SpriteComponent>(explosion);
    fxSprite.setTexture(*textures.get("explosion"));
    registry.emplace<TransformComponent>(explosion);
    registry.emplace<LayerComponent>(explosion,
        LayerComponent::create(RenderLayer::Effects));

    // 5. HUD layer - health bar
    EntityId healthBar = registry.createEntity();
    auto& hudSprite = registry.emplace<SpriteComponent>(healthBar);
    hudSprite.setTexture(*textures.get("health_bar"));
    auto& hudTransform = registry.emplace<TransformComponent>(healthBar);
    hudTransform.x = 10.0F;  // Fixed position
    hudTransform.y = 10.0F;
    registry.emplace<LayerComponent>(healthBar,
        LayerComponent::create(RenderLayer::HUD));

    // 6. Debug layer - collision boxes (optional)
    #ifdef DEBUG_DRAW
    EntityId debugBox = registry.createEntity();
    auto& debugSprite = registry.emplace<SpriteComponent>(debugBox);
    debugSprite.setTexture(*textures.get("debug_box"));
    registry.emplace<TransformComponent>(debugBox);
    registry.emplace<LayerComponent>(debugBox,
        LayerComponent::create(RenderLayer::Debug));
    #endif
}
```

## RenderSystem Integration

The `RenderSystem` automatically sorts entities by layer before rendering:

```cpp
// In RenderSystem::update()
// 1. Collect all entities with SpriteComponent + TransformComponent
// 2. Get their LayerComponent (or default to 0)
// 3. Sort by layer (stable_sort)
// 4. Render in order
```

This means you don't need to worry about creation order - entities are always rendered in the correct layer order.

## Best Practices

### Layer Organization

Group related entities in the same layer range:

```cpp
namespace GameLayers
{
    // Background
    constexpr int FAR_BG   = -150;
    constexpr int MID_BG   = -100;
    constexpr int NEAR_BG  = -50;

    // Entities
    constexpr int GROUND   = -10;
    constexpr int PLAYER   = 0;
    constexpr int ENEMY    = 5;
    constexpr int BULLET   = 10;

    // Effects
    constexpr int PARTICLE = 50;
    constexpr int EXPLOSION = 55;

    // UI
    constexpr int UI_BG    = 100;
    constexpr int UI_ICON  = 110;
    constexpr int UI_TEXT  = 120;
}
```

### HUD Elements

Always use high layer values for HUD to ensure they appear on top:

```cpp
// Good
registry.emplace<LayerComponent>(hud, LayerComponent::create(RenderLayer::HUD));

// Bad - might be covered by effects
registry.emplace<LayerComponent>(hud, LayerComponent::create(RenderLayer::Entities));
```

### Parallax Backgrounds

Use multiple background layers with different scroll speeds:

```cpp
EntityId farBg = registry.createEntity();
// ...
registry.emplace<LayerComponent>(farBg, LayerComponent::create(RenderLayer::Background));
registry.emplace<VelocityComponent>(farBg, 0.2F, 0.0F);  // Slow

EntityId nearBg = registry.createEntity();
// ...
registry.emplace<LayerComponent>(nearBg, LayerComponent::create(RenderLayer::Midground));
registry.emplace<VelocityComponent>(nearBg, 0.5F, 0.0F);  // Faster
```

### Dynamic Layer Changes

You can change an entity's layer at runtime:

```cpp
auto& layer = registry.get<LayerComponent>(entity);
layer.layer = RenderLayer::Effects;  // Move to effects layer
```

## Performance

- **Sorting**: O(n log n) per frame where n = visible entities
- **Stable Sort**: Preserves order of entities within the same layer
- **Optimization**: Consider batching entities by texture within each layer

## Common Patterns

### Scene Layers

```cpp
// Level background
layerBackground = LayerComponent::create(RenderLayer::Background);

// Game world
layerEntities = LayerComponent::create(RenderLayer::Entities);

// Effects over entities
layerEffects = LayerComponent::create(RenderLayer::Effects);

// UI always on top
layerUI = LayerComponent::create(RenderLayer::UI);
```

### Debug Visualization

```cpp
#ifdef DEBUG_MODE
EntityId debugEntity = registry.createEntity();
// ... setup ...
registry.emplace<LayerComponent>(debugEntity,
    LayerComponent::create(RenderLayer::Debug));
#endif
```

## Troubleshooting

### Entity Not Rendering

Check layer values:
```cpp
if (registry.has<LayerComponent>(entity)) {
    auto& layer = registry.get<LayerComponent>(entity);
    std::cout << "Entity layer: " << layer.layer << std::endl;
}
```

### Wrong Render Order

Verify layer constants are in correct order:
```cpp
assert(RenderLayer::Background < RenderLayer::Entities);
assert(RenderLayer::Entities < RenderLayer::HUD);
```

### HUD Behind Entities

Make sure HUD uses a high layer value:
```cpp
// Should be > 100
registry.emplace<LayerComponent>(hud,
    LayerComponent::create(RenderLayer::HUD));
```
