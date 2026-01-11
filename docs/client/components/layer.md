# LayerComponent

## Overview

The `LayerComponent` is a rendering organization component in the R-Type client that controls the draw order of entities through a layering system. It enables proper visual stacking of game elements, ensuring backgrounds render behind gameplay entities, which render behind UI elements, creating proper visual depth in the 2D game world.

## Purpose and Design Philosophy

2D games require careful management of draw order:

1. **Visual Hierarchy**: Backgrounds behind entities behind UI
2. **Parallax Depth**: Multiple background layers at different depths
3. **Effect Layering**: Explosions and effects above entities
4. **Debug Overlays**: Debug info on top of everything
5. **HUD Separation**: UI stays above game world

The component uses signed integers for layers, allowing negative values for backgrounds and positive for foreground elements, with zero as the default entity layer.

## Component Structure

```cpp
namespace RenderLayer
{
    constexpr int Background = -100;
    constexpr int Midground  = -50;
    constexpr int Entities   = 0;
    constexpr int Effects    = 50;
    constexpr int UI         = 100;
    constexpr int HUD        = 150;
    constexpr int Debug      = 200;
}

struct LayerComponent
{
    int layer = RenderLayer::Entities;

    static LayerComponent create(int layer);
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `layer` | `int` | `0` (Entities) | The render layer value. Lower values render first (behind), higher values render last (in front). |

### Predefined Layers

| Layer | Value | Purpose |
|-------|-------|---------|
| `Background` | `-100` | Far background elements |
| `Midground` | `-50` | Mid-distance parallax layers |
| `Entities` | `0` | Players, enemies, projectiles |
| `Effects` | `50` | Explosions, particles, screen effects |
| `UI` | `100` | Menus, dialog boxes |
| `HUD` | `150` | Health bars, score, ammo count |
| `Debug` | `200` | Hitbox visualization, FPS counter |

## Factory Method

### `create(int layer)`

Creates a LayerComponent with the specified layer.

```cpp
static LayerComponent create(int layer)
{
    LayerComponent l;
    l.layer = layer;
    return l;
}
```

**Example:**
```cpp
// Background entity
auto bgLayer = LayerComponent::create(RenderLayer::Background);

// Default entity layer
auto entityLayer = LayerComponent::create(RenderLayer::Entities);

// Custom layer between entities and effects
auto customLayer = LayerComponent::create(25);
```

## Usage Patterns

### Complete Entity Setup

```cpp
Entity createPlayer(Registry& registry, float x, float y)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    registry.addComponent(entity, SpriteComponent::create("player_ship"));
    registry.addComponent(entity, LayerComponent::create(RenderLayer::Entities));
    
    return entity;
}

Entity createExplosion(Registry& registry, float x, float y)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    registry.addComponent(entity, SpriteComponent::create("explosion"));
    registry.addComponent(entity, AnimationComponent::create("explosion_anim", false));
    registry.addComponent(entity, LayerComponent::create(RenderLayer::Effects));
    
    return entity;
}

Entity createHealthBar(Registry& registry)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(20, 20));
    registry.addComponent(entity, BoxComponent::create(200, 30, Color{50, 50, 60}, Color{80, 80, 100}));
    registry.addComponent(entity, LayerComponent::create(RenderLayer::HUD));
    
    return entity;
}
```

### Parallax Background Setup

```cpp
void createParallaxBackground(Registry& registry)
{
    // Far stars (slowest, most distant)
    auto farStars = registry.createEntity();
    registry.addComponent(farStars, SpriteComponent::create("stars_far"));
    registry.addComponent(farStars, BackgroundScrollComponent::create(20.0F, 0.0F, 1920.0F));
    registry.addComponent(farStars, LayerComponent::create(RenderLayer::Background - 20));
    
    // Nebula (medium distance)
    auto nebula = registry.createEntity();
    registry.addComponent(nebula, SpriteComponent::create("nebula"));
    registry.addComponent(nebula, BackgroundScrollComponent::create(50.0F, 0.0F, 1920.0F));
    registry.addComponent(nebula, LayerComponent::create(RenderLayer::Background));
    
    // Asteroid field (closer)
    auto asteroids = registry.createEntity();
    registry.addComponent(asteroids, SpriteComponent::create("asteroids"));
    registry.addComponent(asteroids, BackgroundScrollComponent::create(100.0F, 0.0F, 1920.0F));
    registry.addComponent(asteroids, LayerComponent::create(RenderLayer::Midground));
}
```

## RenderSystem Integration

The render system sorts entities by layer:

```cpp
void RenderSystem::render(Registry& registry, Window& window)
{
    // Collect all renderable entities with transforms and layers
    std::vector<RenderData> renderables;
    
    auto view = registry.view<SpriteComponent, TransformComponent>();
    for (auto entity : view) {
        RenderData data;
        data.entity = entity;
        data.transform = &view.get<TransformComponent>(entity);
        data.sprite = &view.get<SpriteComponent>(entity);
        
        // Get layer, default to Entities if not present
        if (registry.hasComponent<LayerComponent>(entity)) {
            data.layer = registry.getComponent<LayerComponent>(entity).layer;
        } else {
            data.layer = RenderLayer::Entities;
        }
        
        renderables.push_back(data);
    }
    
    // Sort by layer (lower layers render first = behind)
    std::sort(renderables.begin(), renderables.end(), [](const auto& a, const auto& b) {
        return a.layer < b.layer;
    });
    
    // Render in sorted order
    for (const auto& data : renderables) {
        drawSprite(*data.sprite, *data.transform);
    }
}
```

### Optimized Batch Rendering

For better performance with many entities:

```cpp
void RenderSystem::renderOptimized(Registry& registry, Window& window)
{
    // Pre-allocated buckets for common layers
    std::array<std::vector<Entity>, 7> layerBuckets;
    static constexpr int layerOffsets[] = {-100, -50, 0, 50, 100, 150, 200};
    
    auto view = registry.view<SpriteComponent, TransformComponent, LayerComponent>();
    
    // Sort entities into buckets
    for (auto entity : view) {
        int layer = view.get<LayerComponent>(entity).layer;
        
        // Find appropriate bucket
        for (int i = 0; i < 7; ++i) {
            if (layer <= layerOffsets[i]) {
                layerBuckets[i].push_back(entity);
                break;
            }
        }
    }
    
    // Render each bucket
    for (const auto& bucket : layerBuckets) {
        for (auto entity : bucket) {
            auto& sprite = view.get<SpriteComponent>(entity);
            auto& transform = view.get<TransformComponent>(entity);
            drawSprite(sprite, transform);
        }
    }
}
```

## Layer Ordering Examples

### Complete R-Type Scene Stack

```
┌─────────────────────────────────────────────────┐
│ [Debug: 200]       FPS: 60  Entities: 47        │
├─────────────────────────────────────────────────┤
│ [HUD: 150]         SCORE: 12500    ♥ ♥ ♥        │
├─────────────────────────────────────────────────┤
│ [UI: 100]          (Pause menu if active)       │
├─────────────────────────────────────────────────┤
│ [Effects: 50]      💥 🌟 ✨ (explosions)        │
├─────────────────────────────────────────────────┤
│ [Entities: 0]      🚀 👾 👾 🔫 (player/enemies) │
├─────────────────────────────────────────────────┤
│ [Midground: -50]   (close asteroids)            │
├─────────────────────────────────────────────────┤
│ [Background: -100] (stars, nebulae)             │
└─────────────────────────────────────────────────┘
```

### Sub-Layer Organization

For finer control within standard layers:

```cpp
namespace EntitySubLayer
{
    constexpr int PlayerProjectiles = RenderLayer::Entities + 2;
    constexpr int Player = RenderLayer::Entities + 1;
    constexpr int Enemies = RenderLayer::Entities;
    constexpr int EnemyProjectiles = RenderLayer::Entities - 1;
    constexpr int Pickups = RenderLayer::Entities - 2;
}
```

## Dynamic Layer Changes

Entities can change layers at runtime:

```cpp
void bringToFront(Registry& registry, Entity entity)
{
    if (registry.hasComponent<LayerComponent>(entity)) {
        auto& layer = registry.getComponent<LayerComponent>(entity);
        layer.layer = RenderLayer::Effects;  // Move to effects layer
    }
}

void onPlayerDeath(Registry& registry, Entity player)
{
    // Move player behind new game elements during death animation
    if (registry.hasComponent<LayerComponent>(player)) {
        auto& layer = registry.getComponent<LayerComponent>(player);
        layer.layer = RenderLayer::Midground;
    }
}
```

## Best Practices

1. **Use Predefined Constants**: Prefer `RenderLayer::*` over magic numbers
2. **Consistent Layering**: All entities of same type should share layers
3. **Leave Gaps**: Layer values have gaps for sub-layering needs
4. **Default Layer**: Entities without LayerComponent default to 0
5. **Debug Last**: Debug overlays should always be highest layer

## Performance Considerations

- **Sorting Cost**: Sorting hundreds of entities is fast with std::sort
- **Batch by Layer**: Minimize draw call changes within same layer
- **Camera Culling**: Cull off-screen entities before layer sorting
- **Sparse Data**: Many entities may not need explicit LayerComponent

## Common Layer Mistakes

| Mistake | Problem | Solution |
|---------|---------|----------|
| HUD with camera | HUD scrolls with world | Use separate view or layer check |
| Effects behind entities | Explosions hidden | Use higher layer value |
| Missing player layer | Player renders randomly | Always explicitly set player layer |
| Too many unique layers | Sorting overhead | Consolidate into standard layers |

## Related Components

- [SpriteComponent](sprite-component.md) - Visual to render
- [TransformComponent](../server/components/transform-component.md) - Position to render at
- [CameraComponent](camera-component.md) - May affect certain layers differently

## Related Systems

- [RenderSystem](../systems/render-system.md) - Uses layer for sorting
- [CameraSystem](../systems/camera-system.md) - Applies camera transforms
