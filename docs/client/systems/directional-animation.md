# DirectionalAnimationSystem

## Overview

The `DirectionalAnimationSystem` is a specialized animation management system in the R-Type client that handles directional sprite animations based on entity velocity. It implements a state machine that smoothly transitions between idle, upward, and downward animation states, providing visual feedback for vertical movement particularly important for player ships and certain enemy types.

## Purpose and Design Philosophy

Directional animations add visual polish and player feedback:

1. **Movement Indication**: Visual cues for current movement direction
2. **State Machine**: Smooth transitions prevent jarring animation changes
3. **Phase Timing**: Controlled transition durations for natural motion
4. **Clip Resolution**: Efficient lookup of animation data
5. **Frame Application**: Coordinates with sprite rendering

The system reads velocity to determine intent and manages a seven-phase state machine for professional-quality transitions.

## Class Definition

```cpp
class DirectionalAnimationSystem : public ISystem
{
  public:
    DirectionalAnimationSystem(AnimationRegistry& animations, AnimationLabels& labels);
    void update(Registry& registry, float deltaTime) override;

  private:
    void applyClipFrame(Registry& registry, EntityId id, 
                        const AnimationClip& clip, std::uint32_t frameIndex);
    
    struct Clips
    {
        const AnimationClip* idle{nullptr};
        const AnimationClip* up{nullptr};
        const AnimationClip* down{nullptr};
    };
    
    Clips resolveClips(const DirectionalAnimationComponent& dirAnim) const;
    std::pair<bool, bool> intents(const VelocityComponent& vel, float threshold) const;
    
    void handlePhase(Registry& registry, EntityId id, 
                     DirectionalAnimationComponent& dirAnim,
                     const Clips& clips, bool upIntent, bool downIntent, 
                     float deltaTime);
    
    // Phase handlers
    void handleUpIn(Registry& registry, EntityId id, ...);
    void handleUpHold(Registry& registry, EntityId id, ...);
    void handleUpOut(Registry& registry, EntityId id, ...);
    void handleDownIn(Registry& registry, EntityId id, ...);
    void handleDownHold(Registry& registry, EntityId id, ...);
    void handleDownOut(Registry& registry, EntityId id, ...);
    
    void startIdle(Registry& registry, EntityId id, ...);
    void startUpIn(Registry& registry, EntityId id, ...);
    void startDownIn(Registry& registry, EntityId id, ...);
    
    AnimationRegistry* animations_{nullptr};
    AnimationLabels* labels_{nullptr};
};
```

## Constructor

```cpp
DirectionalAnimationSystem(AnimationRegistry& animations, AnimationLabels& labels);
```

**Parameters:**
- `animations`: Registry containing animation clip definitions
- `labels`: Label mapper for resolving animation names to clips

**Example:**
```cpp
DirectionalAnimationSystem dirAnimSystem(animationRegistry, animationLabels);
scheduler.addSystem(&dirAnimSystem);
```

## State Machine

### Seven-Phase Model

```
                    ┌─────────────────────────────────────┐
                    │                                     │
                    ▼                                     │
            ┌───────────────┐    Timer Complete          │
            │     Idle      │◄───────────────────────────┤
            └───────┬───────┘                            │
                    │                                     │
        ┌───────────┼───────────┐                        │
        │ Up Intent │           │ Down Intent            │
        ▼           │           ▼                        │
   ┌─────────┐      │      ┌──────────┐                  │
   │  UpIn   │──────┤      │  DownIn  │──────────────────┤
   └────┬────┘      │      └────┬─────┘                  │
        │ Timer     │           │ Timer                  │
        ▼           │           ▼                        │
   ┌─────────┐      │      ┌──────────┐                  │
   │ UpHold  │      │      │ DownHold │                  │
   └────┬────┘      │      └────┬─────┘                  │
        │ No Intent │           │ No Intent              │
        ▼           │           ▼                        │
   ┌─────────┐      │      ┌──────────┐                  │
   │  UpOut  │──────┘      │ DownOut  │──────────────────┘
   └─────────┘             └──────────┘
```

### Phase Definitions

| Phase | Description | Duration | Entry Condition |
|-------|-------------|----------|-----------------|
| Idle | Neutral animation | Indefinite | No vertical input |
| UpIn | Transition to up tilt | ~100ms | Up intent from Idle |
| UpHold | Holding up tilt | While input held | UpIn complete |
| UpOut | Returning from up | ~100ms | No up intent |
| DownIn | Transition to down tilt | ~100ms | Down intent from Idle |
| DownHold | Holding down tilt | While input held | DownIn complete |
| DownOut | Returning from down | ~100ms | No down intent |

## Main Update Loop

```cpp
void DirectionalAnimationSystem::update(Registry& registry, float deltaTime)
{
    auto view = registry.view<DirectionalAnimationComponent, VelocityComponent>();
    
    for (auto entity : view) {
        auto& dirAnim = view.get<DirectionalAnimationComponent>(entity);
        auto& velocity = view.get<VelocityComponent>(entity);
        
        // Resolve animation clips for this entity
        Clips clips = resolveClips(dirAnim);
        if (!clips.idle) continue;  // No valid animations
        
        // Determine movement intents
        auto [upIntent, downIntent] = intents(velocity, dirAnim.threshold);
        
        // Update phase timer
        if (dirAnim.phaseTime >= 0) {
            dirAnim.phaseTime += deltaTime;
        }
        
        // Process current phase
        handlePhase(registry, entity, dirAnim, clips, upIntent, downIntent, deltaTime);
    }
}
```

## Intent Detection

```cpp
std::pair<bool, bool> DirectionalAnimationSystem::intents(
    const VelocityComponent& vel, float threshold) const
{
    bool upIntent = vel.y < -threshold;    // Negative Y = moving up
    bool downIntent = vel.y > threshold;   // Positive Y = moving down
    return {upIntent, downIntent};
}
```

## Phase Handlers

### Idle Phase

```cpp
void DirectionalAnimationSystem::handleIdle(Registry& registry, EntityId id,
                                             DirectionalAnimationComponent& dirAnim,
                                             const Clips& clips,
                                             bool upIntent, bool downIntent)
{
    // Apply idle animation
    if (clips.idle) {
        uint32_t frame = calculateIdleFrame(dirAnim.phaseTime, clips.idle);
        applyClipFrame(registry, id, *clips.idle, frame);
    }
    
    // Check for transition
    if (upIntent && clips.up) {
        startUpIn(registry, id, dirAnim, clips);
    } else if (downIntent && clips.down) {
        startDownIn(registry, id, dirAnim, clips);
    }
}
```

### Up-In Phase (Transition)

```cpp
void DirectionalAnimationSystem::handleUpIn(Registry& registry, EntityId id,
                                             DirectionalAnimationComponent& dirAnim,
                                             const Clips& clips,
                                             bool downIntent, float deltaTime)
{
    constexpr float TRANSITION_DURATION = 0.1F;  // 100ms
    
    // Calculate transition progress
    float progress = dirAnim.phaseTime / TRANSITION_DURATION;
    
    if (progress >= 1.0F) {
        // Transition complete, enter hold
        dirAnim.phase = DirectionalAnimationComponent::Phase::UpHold;
        dirAnim.phaseTime = 0.0F;
        return;
    }
    
    // Blend frame between idle and up
    // For simplicity, use last idle frame transitioning to first up frame
    uint32_t frame = static_cast<uint32_t>(progress * clips.up->frameCount);
    applyClipFrame(registry, id, *clips.up, frame);
    
    // Allow interrupt to down
    if (downIntent && clips.down) {
        startDownIn(registry, id, dirAnim, clips);
    }
}
```

### Up-Hold Phase

```cpp
void DirectionalAnimationSystem::handleUpHold(Registry& registry, EntityId id,
                                               DirectionalAnimationComponent& dirAnim,
                                               const Clips& clips,
                                               bool upIntent, bool downIntent)
{
    // Play up animation loop
    if (clips.up) {
        uint32_t frame = calculateLoopFrame(dirAnim.phaseTime, clips.up);
        applyClipFrame(registry, id, *clips.up, frame);
    }
    
    // Check for exit conditions
    if (!upIntent) {
        if (downIntent && clips.down) {
            // Direct transition to down
            startDownIn(registry, id, dirAnim, clips);
        } else {
            // Return to idle
            dirAnim.phase = DirectionalAnimationComponent::Phase::UpOut;
            dirAnim.phaseTime = 0.0F;
        }
    }
}
```

### Up-Out Phase (Return Transition)

```cpp
void DirectionalAnimationSystem::handleUpOut(Registry& registry, EntityId id,
                                              DirectionalAnimationComponent& dirAnim,
                                              const Clips& clips,
                                              float deltaTime)
{
    constexpr float TRANSITION_DURATION = 0.1F;
    
    float progress = dirAnim.phaseTime / TRANSITION_DURATION;
    
    if (progress >= 1.0F) {
        // Return to idle
        startIdle(registry, id, dirAnim, clips);
        return;
    }
    
    // Reverse blend: up back to idle
    uint32_t maxFrame = clips.up->frameCount - 1;
    uint32_t frame = maxFrame - static_cast<uint32_t>(progress * maxFrame);
    applyClipFrame(registry, id, *clips.up, frame);
}
```

## Clip Resolution

```cpp
DirectionalAnimationSystem::Clips DirectionalAnimationSystem::resolveClips(
    const DirectionalAnimationComponent& dirAnim) const
{
    Clips clips;
    
    // Resolve animation labels to actual clips
    if (labels_) {
        clips.idle = animations_->getClip(labels_->resolve(dirAnim.idleLabel));
        clips.up = animations_->getClip(labels_->resolve(dirAnim.upLabel));
        clips.down = animations_->getClip(labels_->resolve(dirAnim.downLabel));
    }
    
    return clips;
}
```

## Frame Application

```cpp
void DirectionalAnimationSystem::applyClipFrame(Registry& registry, EntityId id,
                                                  const AnimationClip& clip,
                                                  std::uint32_t frameIndex)
{
    if (!registry.hasComponent<SpriteComponent>(id)) return;
    
    auto& sprite = registry.getComponent<SpriteComponent>(id);
    
    // Clamp frame to valid range
    frameIndex = std::min(frameIndex, clip.frameCount - 1);
    
    // Get frame from clip
    const auto& frame = clip.frames[frameIndex];
    
    // Apply to sprite
    sprite.frameX = frame.column;
    sprite.frameY = frame.row;
    sprite.textureId = clip.textureId;
}
```

## Animation Labels Configuration

```json
{
    "labels": {
        "player_idle": "atlas_player:idle",
        "player_up": "atlas_player:tilt_up",
        "player_down": "atlas_player:tilt_down"
    },
    "clips": {
        "atlas_player:idle": {
            "texture": "player_ship",
            "frames": [[0,0], [1,0]],
            "frameRate": 8
        },
        "atlas_player:tilt_up": {
            "texture": "player_ship",
            "frames": [[2,0], [3,0], [4,0]],
            "frameRate": 12
        },
        "atlas_player:tilt_down": {
            "texture": "player_ship",
            "frames": [[5,0], [6,0], [7,0]],
            "frameRate": 12
        }
    }
}
```

## Usage Example

```cpp
// Setup player with directional animation
Entity createPlayer(Registry& registry)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(100, 300));
    registry.addComponent(entity, VelocityComponent::create(0, 0));
    registry.addComponent(entity, SpriteComponent::create("player_ship"));
    
    DirectionalAnimationComponent dirAnim;
    dirAnim.spriteId = "player_ship";
    dirAnim.idleLabel = "player_idle";
    dirAnim.upLabel = "player_up";
    dirAnim.downLabel = "player_down";
    dirAnim.threshold = 50.0F;
    registry.addComponent(entity, dirAnim);
    
    return entity;
}
```

## Best Practices

1. **Threshold Tuning**: Adjust threshold to avoid triggering on small movements
2. **Transition Timing**: Keep transitions short (50-150ms) for responsiveness
3. **Frame Rate Matching**: Ensure transition duration matches animation frames
4. **Interrupt Handling**: Allow smooth interrupts between up and down
5. **Fallback Animations**: Always provide idle as fallback

## Performance Considerations

- **Clip Caching**: Resolved clips can be cached per entity
- **Sparse Updates**: Only process entities with DirectionalAnimationComponent
- **Efficient Lookups**: AnimationLabels uses hash maps

## Related Components

- [DirectionalAnimationComponent](../components/directional-animation-component.md) - State data
- [VelocityComponent](../server/components/velocity-component.md) - Movement input
- [SpriteComponent](../components/sprite-component.md) - Visual output

## Related Systems

- [AnimationSystem](animation-system.md) - General frame animation
- [RenderSystem](render-system.md) - Sprite display
