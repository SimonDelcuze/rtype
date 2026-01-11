# BossComponent

## Overview

The `BossComponent` is a marker component used within the R-Type client to identify and manage boss entities. Boss encounters are a fundamental gameplay element in shoot-em-up games, representing climactic battles at the end of levels or wave sequences. This component provides essential metadata for boss entities, enabling specialized rendering, UI integration, and gameplay mechanics specific to boss fights.

## Purpose and Design Philosophy

Boss battles require special treatment in the game engine for several reasons:

1. **UI Integration**: Boss health bars, names, and special indicators need to be displayed
2. **Camera Behavior**: The camera may need to adjust its behavior during boss encounters
3. **Audio Cues**: Special music and sound effects may trigger when engaging bosses
4. **Spawn Logic**: Boss entities often have unique spawning and death sequences
5. **Event Triggers**: Boss defeats typically trigger level progression events

The `BossComponent` serves as a lightweight identifier that allows systems to query for and operate on boss entities specifically, without coupling the boss logic to other game systems.

## Component Structure

```cpp
struct BossComponent
{
    std::string name;

    static BossComponent create(const std::string& name = "Boss");
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `name` | `std::string` | `"Boss"` | The display name of the boss, used for UI labels, debug logging, and localization references. |

## Factory Method

### `create(const std::string& name = "Boss")`

Creates a new `BossComponent` with the specified boss name.

**Parameters:**
- `name`: The display name for the boss entity (optional, defaults to "Boss")

**Returns:** A configured `BossComponent` instance.

**Example:**
```cpp
// Create a named boss component
auto bossComp = BossComponent::create("Gomander");

// Create with default name
auto genericBoss = BossComponent::create();
```

## Usage Patterns

### Spawning a Boss Entity

When creating a boss during level gameplay:

```cpp
void spawnBoss(Registry& registry, const LevelBossData& bossData)
{
    auto entity = registry.createEntity();
    
    // Add the boss marker component
    registry.addComponent(entity, BossComponent::create(bossData.name));
    
    // Add visual components
    registry.addComponent(entity, SpriteComponent::create(bossData.spriteId));
    registry.addComponent(entity, AnimationComponent::create(bossData.animationId));
    
    // Add gameplay components
    registry.addComponent(entity, TransformComponent::create(bossData.spawnX, bossData.spawnY));
    registry.addComponent(entity, HealthComponent::create(bossData.maxHealth));
    registry.addComponent(entity, HitboxComponent::create(bossData.hitboxWidth, bossData.hitboxHeight));
    
    // Place in entity layer
    registry.addComponent(entity, LayerComponent::create(RenderLayer::Entities));
}
```

### Querying for Boss Entities

Systems can easily identify boss entities for special processing:

```cpp
void BossHealthBarSystem::update(Registry& registry, float deltaTime)
{
    auto view = registry.view<BossComponent, HealthComponent, TransformComponent>();
    
    for (auto entity : view) {
        auto& boss = view.get<BossComponent>(entity);
        auto& health = view.get<HealthComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        
        // Render boss health bar with boss name
        renderBossHealthBar(boss.name, health.current, health.max, transform.x, transform.y);
    }
}
```

### Boss Defeat Detection

Detecting when a boss has been defeated:

```cpp
void checkBossDefeat(Registry& registry, EventBus& eventBus)
{
    auto view = registry.view<BossComponent, HealthComponent>();
    
    for (auto entity : view) {
        auto& health = view.get<HealthComponent>(entity);
        auto& boss = view.get<BossComponent>(entity);
        
        if (health.current <= 0) {
            // Publish boss defeat event
            eventBus.publish(BossDefeatedEvent{
                .entityId = entity,
                .bossName = boss.name
            });
            
            // Mark entity for destruction
            registry.markForDestruction(entity);
        }
    }
}
```

### Audio System Integration

Triggering boss music when a boss is present:

```cpp
void AudioSystem::checkBossMusic(Registry& registry)
{
    auto bosses = registry.view<BossComponent>();
    
    if (!bosses.empty() && !m_bossMusic.isPlaying()) {
        m_backgroundMusic.pause();
        m_bossMusic.play();
    } else if (bosses.empty() && m_bossMusic.isPlaying()) {
        m_bossMusic.stop();
        m_backgroundMusic.resume();
    }
}
```

## Integration with HUD System

The `HUDSystem` uses `BossComponent` to display boss-specific UI elements:

```cpp
void HUDSystem::renderBossUI(Registry& registry)
{
    auto view = registry.view<BossComponent, HealthComponent>();
    
    for (auto entity : view) {
        auto& boss = view.get<BossComponent>(entity);
        auto& health = view.get<HealthComponent>(entity);
        
        // Display boss name at top of screen
        renderCenteredText(boss.name, BOSS_NAME_Y_POSITION, BOSS_NAME_FONT_SIZE);
        
        // Display boss health bar
        float healthPercent = health.current / health.max;
        renderBossHealthBar(healthPercent, BOSS_HEALTH_BAR_Y);
    }
}
```

## Level Integration

Bosses are typically defined in level JSON files and spawned by the level system:

```json
{
    "bosses": [
        {
            "name": "Dobkeratops",
            "sprite": "boss_dobkeratops",
            "health": 5000,
            "spawnTime": 180.0,
            "spawnX": 1200,
            "spawnY": 300
        }
    ]
}
```

## Classic R-Type Boss Names

For reference, here are some classic R-Type boss names that might be used:

- Dobkeratops (Level 1)
- Gomander (Level 2)
- Compiler (Level 3)
- Bydo Core (Level 4)
- Subkeratom (Level 5)

## Best Practices

1. **Single Boss Per Area**: Typically only one boss should be active at a time per level section
2. **Unique Names**: Use descriptive, unique names for each boss for debugging and UI clarity
3. **Cleanup**: Ensure boss entities are properly destroyed after defeat to prevent audio/UI issues
4. **Event-Driven**: Use the event bus to communicate boss state changes rather than tight coupling

## Performance Considerations

- **Lightweight Marker**: The component contains only a string, minimal memory overhead
- **Sparse Usage**: Only a few entities will have this component at any time
- **Query Optimization**: Systems can early-exit if no boss entities exist

## Related Components

- [HealthComponent](../server/components/health-component.md) - Tracks boss health
- [HitboxComponent](../server/components/hitbox-component.md) - Defines boss collision areas
- [AnimationComponent](animation-component.md) - Boss visual animations

## Related Systems

- [HUDSystem](../systems/hud-system.md) - Displays boss health bars and names
- [AudioSystem](../systems/audio-system.md) - Manages boss encounter music
- [LevelEventSystem](../systems/level-event-system.md) - Handles boss spawn/defeat events
