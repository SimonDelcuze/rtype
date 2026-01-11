# FollowerFacingComponent

## Overview

The `FollowerFacingComponent` is a visual enhancement component in the R-Type client that manages the horizontal facing direction of follower-type enemies. This component ensures that chasing enemies visually face the correct direction based on their movement or target position, adding visual polish to enemy AI behavior.

## Purpose and Design Philosophy

In shoot'em-up games, enemies that follow or chase players should visually indicate their movement direction:

1. **Visual Clarity**: Players can quickly assess enemy movement intent
2. **Animation Polish**: Prevents enemies from appearing to move backward
3. **Sprite Flipping**: Handles horizontal sprite mirroring automatically
4. **State Persistence**: Tracks whether direction has been initialized

The component is lightweight, storing only the facing state without any logic. The `FollowerFacingSystem` handles the actual direction updates and sprite flipping.

## Component Structure

```cpp
struct FollowerFacingComponent
{
    bool initialized = false;
    bool facingRight = false;
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `initialized` | `bool` | `false` | Whether the facing direction has been set. Used to handle initial spawn direction. |
| `facingRight` | `bool` | `false` | Whether the entity is currently facing right. When false, faces left (toward players coming from the left side). |

## Usage Patterns

### Creating a Follower Enemy

```cpp
Entity spawnFollower(Registry& registry, float x, float y, Entity targetPlayer)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    registry.addComponent(entity, VelocityComponent::create(0.0F, 0.0F));
    
    // Chaser AI that follows player
    registry.addComponent(entity, AIFollowerComponent::create(targetPlayer, 120.0F));
    
    // Sprite (default facing left toward player spawn)
    registry.addComponent(entity, SpriteComponent::create("follower_enemy", 48, 32));
    
    // Follower facing for sprite flipping
    registry.addComponent(entity, FollowerFacingComponent{});
    
    registry.addComponent(entity, LayerComponent::create(RenderLayer::Entities));
    
    return entity;
}
```

### Level JSON Definition

Follower enemies defined in level data:

```json
{
    "enemies": [
        {
            "type": "follower",
            "spawnTime": 15.0,
            "position": {"x": 1200, "y": 300},
            "components": {
                "ai": {"type": "follow", "speed": 100},
                "facing": true
            }
        }
    ]
}
```

## FollowerFacingSystem Logic

The system updates facing based on velocity or target:

```cpp
void FollowerFacingSystem::update(Registry& registry, float deltaTime)
{
    auto view = registry.view<FollowerFacingComponent, VelocityComponent, SpriteComponent>();
    
    for (auto entity : view) {
        auto& facing = view.get<FollowerFacingComponent>(entity);
        auto& velocity = view.get<VelocityComponent>(entity);
        auto& sprite = view.get<SpriteComponent>(entity);
        
        // Only update if there's significant horizontal movement
        constexpr float VELOCITY_THRESHOLD = 5.0F;
        
        if (std::abs(velocity.x) > VELOCITY_THRESHOLD) {
            bool shouldFaceRight = velocity.x > 0;
            
            // Check if direction changed or first initialization
            if (!facing.initialized || facing.facingRight != shouldFaceRight) {
                facing.initialized = true;
                facing.facingRight = shouldFaceRight;
                
                // Update sprite flipping
                sprite.flipHorizontal = shouldFaceRight;
                // Note: Default sprite faces left, so flip when facing right
            }
        }
    }
}
```

### Target-Based Facing

Alternative implementation based on target position:

```cpp
void FollowerFacingSystem::updateTargetBased(Registry& registry)
{
    auto view = registry.view<FollowerFacingComponent, AIFollowerComponent, 
                               TransformComponent, SpriteComponent>();
    
    for (auto entity : view) {
        auto& facing = view.get<FollowerFacingComponent>(entity);
        auto& ai = view.get<AIFollowerComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        auto& sprite = view.get<SpriteComponent>(entity);
        
        // Get target position
        if (registry.entityExists(ai.targetEntity)) {
            auto& targetTransform = registry.getComponent<TransformComponent>(ai.targetEntity);
            
            bool shouldFaceRight = targetTransform.x > transform.x;
            
            if (!facing.initialized || facing.facingRight != shouldFaceRight) {
                facing.initialized = true;
                facing.facingRight = shouldFaceRight;
                sprite.flipHorizontal = shouldFaceRight;
            }
        }
    }
}
```

## Sprite Sheet Conventions

For follower enemies, sprites are typically designed facing left:

```
Standard Orientation (facing left toward player):
┌─────────────────┐
│    ◀──  👁️      │  <- Enemy facing left
│    \_/──────    │     (default, no flip)
└─────────────────┘

Flipped Orientation (facing right away from player spawn):
┌─────────────────┐
│      👁️  ──▶    │  <- Enemy facing right
│    ──────\_/    │     (flipHorizontal = true)
└─────────────────┘
```

## Integration with Animation

When combined with directional animations:

```cpp
void updateFollowerAnimation(Registry& registry, Entity entity)
{
    auto& facing = registry.getComponent<FollowerFacingComponent>(entity);
    auto& anim = registry.getComponent<AnimationComponent>(entity);
    
    // Select animation based on facing
    if (facing.facingRight) {
        anim.currentAnimation = "follower_move_right";
    } else {
        anim.currentAnimation = "follower_move_left";
    }
}
```

## Patrol Behavior

For patrolling enemies that change direction:

```cpp
void PatrolSystem::update(Registry& registry)
{
    auto view = registry.view<PatrolComponent, FollowerFacingComponent, 
                               TransformComponent, VelocityComponent>();
    
    for (auto entity : view) {
        auto& patrol = view.get<PatrolComponent>(entity);
        auto& facing = view.get<FollowerFacingComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        auto& velocity = view.get<VelocityComponent>(entity);
        
        // Check if reached patrol boundary
        if (transform.x <= patrol.leftBound) {
            velocity.x = std::abs(velocity.x);  // Move right
            facing.facingRight = true;
        } else if (transform.x >= patrol.rightBound) {
            velocity.x = -std::abs(velocity.x);  // Move left
            facing.facingRight = false;
        }
    }
}
```

## Enemy Types Using This Component

| Enemy Type | Behavior | Facing Logic |
|------------|----------|--------------|
| **Follower** | Chases nearest player | Faces toward velocity direction |
| **Patrol** | Moves between points | Faces movement direction |
| **Ambush** | Waits then attacks | Faces target when attacking |
| **Swarm** | Group movement | Faces swarm movement direction |

## Best Practices

1. **Velocity Threshold**: Use a minimum velocity to avoid jittering between facing states
2. **Initial State**: Set `initialized = false` to allow first-frame calculation
3. **Sprite Design**: Design sprites facing left (toward player spawn)
4. **Animation Consistency**: Ensure animations match facing direction

## Common Issues and Solutions

### Issue: Rapid Flipping

When enemy oscillates around target:

```cpp
// Solution: Add hysteresis
if (!facing.facingRight && velocity.x > VELOCITY_THRESHOLD + HYSTERESIS) {
    facing.facingRight = true;
} else if (facing.facingRight && velocity.x < -VELOCITY_THRESHOLD - HYSTERESIS) {
    facing.facingRight = false;
}
```

### Issue: Wrong Initial Direction

```cpp
// Solution: Initialize based on spawn position relative to center
void initializeFacing(FollowerFacingComponent& facing, float spawnX)
{
    facing.initialized = true;
    facing.facingRight = spawnX < SCREEN_WIDTH / 2;  // Face toward center
}
```

## Performance Considerations

- **Minimal Overhead**: Only two boolean values
- **Conditional Updates**: Only process when velocity exceeds threshold
- **No Memory Allocation**: Stack-allocated, cache-friendly

## Related Components

- [SpriteComponent](sprite-component.md) - Visual representation with flip support
- [VelocityComponent](../server/components/velocity-component.md) - Movement direction
- [AIFollowerComponent](../server/components/monster-ai-component.md) - AI behavior

## Related Systems

- [FollowerFacingSystem](../systems/follower-facing-system.md) - Updates facing direction
- [RenderSystem](../systems/render-system.md) - Applies sprite flipping
