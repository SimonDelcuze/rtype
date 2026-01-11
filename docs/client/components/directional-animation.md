# DirectionalAnimationComponent

## Overview

The `DirectionalAnimationComponent` is a specialized animation control component in the R-Type client that manages directional animation state transitions based on the entity's movement direction. This component is particularly important for player ships and certain enemy types that need to display different visual states when moving up, down, or remaining idle.

## Purpose and Design Philosophy

In classic shoot'em-up games like R-Type, player ships typically tilt or animate differently based on vertical movement direction:

1. **Visual Feedback**: Provides immediate feedback on player input
2. **Immersion**: Makes the ship feel responsive and alive
3. **Anticipation**: Helps players understand their movement state
4. **Polish**: Adds visual quality that distinguishes professional games

The component implements a state machine for smooth transitions between directional states, preventing jarring animation switches.

## Component Structure

```cpp
struct DirectionalAnimationComponent
{
    std::string spriteId;
    std::string idleLabel;
    std::string upLabel;
    std::string downLabel;
    float threshold = 60.0F;

    enum class Phase
    {
        Idle,
        UpIn,
        UpHold,
        UpOut,
        DownIn,
        DownHold,
        DownOut
    };
    Phase phase     = Phase::Idle;
    float phaseTime = -1.0F;
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `spriteId` | `std::string` | `""` | The sprite identifier for animation lookup. |
| `idleLabel` | `std::string` | `""` | Animation label for neutral/idle state. |
| `upLabel` | `std::string` | `""` | Animation label for upward movement state. |
| `downLabel` | `std::string` | `""` | Animation label for downward movement state. |
| `threshold` | `float` | `60.0F` | Velocity threshold to trigger directional animations. |
| `phase` | `Phase` | `Idle` | Current animation phase state. |
| `phaseTime` | `float` | `-1.0F` | Time spent in current phase, used for transitions. |

### Phase Enumeration

The component uses a seven-phase state machine for smooth transitions:

```cpp
enum class Phase
{
    Idle,      // Neutral state, no vertical movement
    UpIn,      // Transitioning into upward tilt
    UpHold,    // Holding upward tilt while moving up
    UpOut,     // Transitioning out of upward tilt
    DownIn,    // Transitioning into downward tilt
    DownHold,  // Holding downward tilt while moving down
    DownOut    // Transitioning out of downward tilt
};
```

## State Machine Flow

The animation phase transitions follow this pattern:

```
                    ┌─────────────────────────────────────┐
                    │                                     │
                    ▼                                     │
            ┌───────────────┐                            │
            │     Idle      │◄───────────────────────────┤
            └───────┬───────┘                            │
                    │                                     │
        ┌───────────┼───────────┐                        │
        │ vel.y < 0 │           │ vel.y > 0              │
        ▼           │           ▼                        │
   ┌─────────┐      │      ┌──────────┐                  │
   │  UpIn   │      │      │  DownIn  │                  │
   └────┬────┘      │      └────┬─────┘                  │
        │           │           │                        │
        ▼           │           ▼                        │
   ┌─────────┐      │      ┌──────────┐                  │
   │ UpHold  │      │      │ DownHold │                  │
   └────┬────┘      │      └────┬─────┘                  │
        │           │           │                        │
        │ vel.y ≥ 0 │           │ vel.y ≤ 0              │
        ▼           │           ▼                        │
   ┌─────────┐      │      ┌──────────┐                  │
   │  UpOut  │──────┘      │ DownOut  │──────────────────┘
   └─────────┘             └──────────┘
```

## Usage Patterns

### Setting Up Player Ship

```cpp
Entity createPlayerShip(Registry& registry, float x, float y)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    registry.addComponent(entity, VelocityComponent::create(0.0F, 0.0F));
    
    // Sprite component for rendering
    registry.addComponent(entity, SpriteComponent::create("player_ship"));
    
    // Animation component for frame animation
    registry.addComponent(entity, AnimationComponent::create("player_idle", true));
    
    // Directional animation for tilt based on movement
    DirectionalAnimationComponent dirAnim;
    dirAnim.spriteId = "player_ship";
    dirAnim.idleLabel = "player_idle";
    dirAnim.upLabel = "player_up";
    dirAnim.downLabel = "player_down";
    dirAnim.threshold = 50.0F;  // Velocity threshold to trigger tilt
    registry.addComponent(entity, dirAnim);
    
    return entity;
}
```

### Animation Labels Configuration

Configure animation labels in assets JSON:

```json
{
    "animations": {
        "player_idle": {
            "sprite": "player_ship",
            "frames": [0, 1],
            "frameRate": 8,
            "loop": true
        },
        "player_up": {
            "sprite": "player_ship",
            "frames": [2, 3, 4],
            "frameRate": 12,
            "loop": true
        },
        "player_down": {
            "sprite": "player_ship",
            "frames": [5, 6, 7],
            "frameRate": 12,
            "loop": true
        },
        "player_up_transition": {
            "sprite": "player_ship",
            "frames": [0, 2],
            "frameRate": 16,
            "loop": false
        },
        "player_down_transition": {
            "sprite": "player_ship",
            "frames": [0, 5],
            "frameRate": 16,
            "loop": false
        }
    }
}
```

## DirectionalAnimationSystem Logic

The system processes directional animation state:

```cpp
void DirectionalAnimationSystem::update(Registry& registry, float deltaTime)
{
    auto view = registry.view<DirectionalAnimationComponent, VelocityComponent, AnimationComponent>();
    
    for (auto entity : view) {
        auto& dirAnim = view.get<DirectionalAnimationComponent>(entity);
        auto& velocity = view.get<VelocityComponent>(entity);
        auto& anim = view.get<AnimationComponent>(entity);
        
        // Update phase time
        if (dirAnim.phaseTime >= 0) {
            dirAnim.phaseTime += deltaTime;
        }
        
        // State machine logic
        switch (dirAnim.phase) {
            case Phase::Idle:
                handleIdlePhase(dirAnim, velocity, anim);
                break;
            case Phase::UpIn:
                handleUpInPhase(dirAnim, velocity, anim, deltaTime);
                break;
            case Phase::UpHold:
                handleUpHoldPhase(dirAnim, velocity, anim);
                break;
            case Phase::UpOut:
                handleUpOutPhase(dirAnim, velocity, anim, deltaTime);
                break;
            case Phase::DownIn:
                handleDownInPhase(dirAnim, velocity, anim, deltaTime);
                break;
            case Phase::DownHold:
                handleDownHoldPhase(dirAnim, velocity, anim);
                break;
            case Phase::DownOut:
                handleDownOutPhase(dirAnim, velocity, anim, deltaTime);
                break;
        }
    }
}

void DirectionalAnimationSystem::handleIdlePhase(DirectionalAnimationComponent& dirAnim,
                                                   const VelocityComponent& velocity,
                                                   AnimationComponent& anim)
{
    if (velocity.y < -dirAnim.threshold) {
        // Start moving up
        dirAnim.phase = Phase::UpIn;
        dirAnim.phaseTime = 0.0F;
        anim.currentAnimation = dirAnim.upLabel + "_transition";
    } else if (velocity.y > dirAnim.threshold) {
        // Start moving down
        dirAnim.phase = Phase::DownIn;
        dirAnim.phaseTime = 0.0F;
        anim.currentAnimation = dirAnim.downLabel + "_transition";
    } else {
        // Stay idle
        if (anim.currentAnimation != dirAnim.idleLabel) {
            anim.currentAnimation = dirAnim.idleLabel;
        }
    }
}

void DirectionalAnimationSystem::handleUpHoldPhase(DirectionalAnimationComponent& dirAnim,
                                                     const VelocityComponent& velocity,
                                                     AnimationComponent& anim)
{
    // Ensure correct animation
    if (anim.currentAnimation != dirAnim.upLabel) {
        anim.currentAnimation = dirAnim.upLabel;
    }
    
    // Check for exit condition
    if (velocity.y >= 0) {
        dirAnim.phase = Phase::UpOut;
        dirAnim.phaseTime = 0.0F;
    }
}
```

## Transition Timing

For smooth visual transitions, the system uses phase timing:

```cpp
constexpr float TRANSITION_IN_DURATION = 0.1F;   // 100ms to enter tilted state
constexpr float TRANSITION_OUT_DURATION = 0.15F; // 150ms to return to idle

void DirectionalAnimationSystem::handleUpInPhase(DirectionalAnimationComponent& dirAnim,
                                                   const VelocityComponent& velocity,
                                                   AnimationComponent& anim,
                                                   float deltaTime)
{
    if (dirAnim.phaseTime >= TRANSITION_IN_DURATION) {
        // Transition complete, enter hold state
        dirAnim.phase = Phase::UpHold;
        dirAnim.phaseTime = -1.0F;
        anim.currentAnimation = dirAnim.upLabel;
    }
}
```

## Advanced Configuration

### Enemy Directional Animation

Enemies that chase players can also use directional animations:

```cpp
DirectionalAnimationComponent createChaserAnimation()
{
    DirectionalAnimationComponent dirAnim;
    dirAnim.spriteId = "chaser_enemy";
    dirAnim.idleLabel = "chaser_neutral";
    dirAnim.upLabel = "chaser_up";
    dirAnim.downLabel = "chaser_down";
    dirAnim.threshold = 30.0F;  // Lower threshold for more responsive animation
    return dirAnim;
}
```

### Force Ship Animation

For the Force pod in R-Type:

```cpp
DirectionalAnimationComponent createForceAnimation()
{
    DirectionalAnimationComponent dirAnim;
    dirAnim.spriteId = "force_pod";
    dirAnim.idleLabel = "force_neutral";
    dirAnim.upLabel = "force_up";
    dirAnim.downLabel = "force_down";
    dirAnim.threshold = 40.0F;
    return dirAnim;
}
```

## Sprite Sheet Layout

Recommended sprite sheet organization for directional animations:

```
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│ I0  │ I1  │ U0  │ U1  │ U2  │ D0  │ D1  │ D2  │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
  │     │     │     │     │     │     │     │
  ▼     ▼     └─────┴─────┘     └─────┴─────┘
 Idle Frames    Up Frames         Down Frames
```

Where:
- `I0-I1`: Idle/neutral animation frames
- `U0-U2`: Upward tilt frames (transition + hold)
- `D0-D2`: Downward tilt frames (transition + hold)

## Best Practices

1. **Smooth Transitions**: Use transition animations rather than instant switches
2. **Velocity Threshold**: Set threshold to avoid jitter from minor movements
3. **Animation Looping**: Hold animations should loop, transitions should not
4. **Frame Rate**: Use faster frame rates for transitions (12-16 FPS)
5. **Symmetry**: Ensure up/down animations are visually symmetric

## Performance Considerations

- **State Check Only**: Only checks velocity threshold, minimal computational cost
- **Animation Change**: Only changes animation when phase changes
- **No Interpolation**: Uses discrete phases, not continuous interpolation

## Related Components

- [AnimationComponent](animation-component.md) - Actual frame animation
- [SpriteComponent](sprite-component.md) - Visual rendering
- [VelocityComponent](../server/components/velocity-component.md) - Movement data

## Related Systems

- [DirectionalAnimationSystem](../systems/directional-animation-system.md) - Processes this component
- [AnimationSystem](../systems/animation-system.md) - Handles frame animation
