# FollowerFacingSystem

## Overview

The `FollowerFacingSystem` is a visual enhancement system in the R-Type client that manages the horizontal facing direction of follower-type enemies based on their velocity. It ensures that chasing enemies visually face the correct direction by flipping their sprites or selecting appropriate animation directions, adding visual polish to enemy AI behavior.

## Purpose and Design Philosophy

Enemies that follow or chase players should indicate their movement direction:

1. **Visual Clarity**: Players can assess enemy movement intent at a glance
2. **Sprite Flipping**: Automatically mirrors sprites based on velocity
3. **Animation Selection**: Chooses left/right animation variants
4. **Threshold Control**: Prevents jitter from small velocity changes
5. **Initialize on First Frame**: Handles initial facing direction

## Class Definition

```cpp
class FollowerFacingSystem : public ISystem
{
  public:
    FollowerFacingSystem(AnimationRegistry& animations, AnimationLabels& labels);
    void initialize() override {}
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override {}

  private:
    AnimationRegistry* animations_ = nullptr;
    AnimationLabels* labels_       = nullptr;

    void applyClipFrame(Registry& registry, EntityId id, 
                        const AnimationClip& clip, std::uint32_t frameIndex) const;
};
```

## Constructor

```cpp
FollowerFacingSystem(AnimationRegistry& animations, AnimationLabels& labels);
```

**Parameters:**
- `animations`: Registry for animation clip data
- `labels`: Mapper for animation label resolution

## Main Update Logic

```cpp
void FollowerFacingSystem::update(Registry& registry, float deltaTime)
{
    auto view = registry.view<FollowerFacingComponent, VelocityComponent, 
                               SpriteComponent, TransformComponent>();
    
    for (auto entity : view) {
        auto& facing = view.get<FollowerFacingComponent>(entity);
        auto& velocity = view.get<VelocityComponent>(entity);
        auto& sprite = view.get<SpriteComponent>(entity);
        
        // Velocity threshold to avoid jitter
        constexpr float VELOCITY_THRESHOLD = 5.0F;
        
        // Only update if significant horizontal movement
        if (std::abs(velocity.x) > VELOCITY_THRESHOLD) {
            bool shouldFaceRight = velocity.x > 0;
            
            // Check if direction changed or first initialization
            if (!facing.initialized || facing.facingRight != shouldFaceRight) {
                facing.initialized = true;
                facing.facingRight = shouldFaceRight;
                
                // Update sprite flipping
                // Default sprites face left (toward player spawn)
                // Flip when facing right (away from player spawn)
                sprite.flipHorizontal = shouldFaceRight;
                
                // Update animation if needed
                updateAnimation(registry, entity, facing.facingRight);
            }
        }
    }
}
```

## Animation Integration

```cpp
void FollowerFacingSystem::updateAnimation(Registry& registry, EntityId entity, bool facingRight)
{
    if (!registry.hasComponent<AnimationComponent>(entity)) return;
    if (!registry.hasComponent<RenderTypeComponent>(entity)) return;
    
    auto& anim = registry.getComponent<AnimationComponent>(entity);
    auto& renderType = registry.getComponent<RenderTypeComponent>(entity);
    
    // Get appropriate animation label based on facing
    std::string label = renderType.baseType;
    if (facingRight) {
        label += "_right";
    } else {
        label += "_left";
    }
    
    // Look up animation clip
    if (labels_) {
        auto resolvedLabel = labels_->resolve(label);
        if (auto* clip = animations_->getClip(resolvedLabel)) {
            applyClipFrame(registry, entity, *clip, 0);
        }
    }
}
```

## Frame Application

```cpp
void FollowerFacingSystem::applyClipFrame(Registry& registry, EntityId id,
                                            const AnimationClip& clip,
                                            std::uint32_t frameIndex) const
{
    if (!registry.hasComponent<SpriteComponent>(id)) return;
    
    auto& sprite = registry.getComponent<SpriteComponent>(id);
    
    // Get frame data
    frameIndex = std::min(frameIndex, static_cast<uint32_t>(clip.frames.size() - 1));
    const auto& frame = clip.frames[frameIndex];
    
    // Apply to sprite
    sprite.textureId = clip.textureId;
    sprite.frameX = frame.column;
    sprite.frameY = frame.row;
}
```

## Sprite Flipping vs Animation Selection

### Method 1: Sprite Flipping (Simpler)

```cpp
// Flip the sprite horizontally
sprite.flipHorizontal = facing.facingRight;
```

**Pros:**
- Single sprite required
- Less memory usage
- Simpler asset management

**Cons:**
- Asymmetric sprites look wrong when flipped
- Text/details may appear reversed

### Method 2: Separate Animations (Better Quality)

```cpp
// Select left or right animation
if (facing.facingRight) {
    anim.currentLabel = "follower_right";
} else {
    anim.currentLabel = "follower_left";
}
```

**Pros:**
- Perfect for asymmetric designs
- Allows different animations per direction
- No visual artifacts

**Cons:**
- Doubles sprite sheet requirements
- More asset management

## Enemy Types Using This System

| Enemy Type | Facing Behavior | Implementation |
|------------|----------------|----------------|
| Follower | Faces movement direction | Velocity-based |
| Patrol | Faces patrol direction | Position-based |
| Seeker | Faces target constantly | Target-based |
| Bouncer | Faces bounce direction | Velocity-based |

## Hysteresis for Stability

Prevent rapid flipping when moving near threshold:

```cpp
void FollowerFacingSystem::updateWithHysteresis(FollowerFacingComponent& facing,
                                                  const VelocityComponent& velocity,
                                                  SpriteComponent& sprite)
{
    constexpr float THRESHOLD = 5.0F;
    constexpr float HYSTERESIS = 10.0F;
    
    if (!facing.initialized) {
        // First frame: use any significant velocity
        if (std::abs(velocity.x) > THRESHOLD) {
            facing.initialized = true;
            facing.facingRight = velocity.x > 0;
            sprite.flipHorizontal = facing.facingRight;
        }
    } else {
        // Subsequent frames: require larger change to flip
        if (facing.facingRight && velocity.x < -THRESHOLD - HYSTERESIS) {
            facing.facingRight = false;
            sprite.flipHorizontal = false;
        } else if (!facing.facingRight && velocity.x > THRESHOLD + HYSTERESIS) {
            facing.facingRight = true;
            sprite.flipHorizontal = true;
        }
    }
}
```

## Usage Example

```cpp
// Create follower enemy with facing
Entity createFollowerEnemy(Registry& registry, float x, float y)
{
    auto entity = registry.createEntity();
    
    registry.addComponent(entity, TransformComponent::create(x, y));
    registry.addComponent(entity, VelocityComponent::create(-100.0F, 0.0F));
    
    // Sprite default faces left
    registry.addComponent(entity, SpriteComponent::create("follower_enemy", 48, 32));
    
    // Animation
    registry.addComponent(entity, AnimationComponent::create("follower_move", true));
    
    // Follower facing for automatic flipping
    registry.addComponent(entity, FollowerFacingComponent{});
    
    // AI component for chase behavior
    registry.addComponent(entity, FollowAIComponent::create(100.0F));
    
    registry.addComponent(entity, LayerComponent::create(RenderLayer::Entities));
    
    return entity;
}
```

## Target-Based Alternative

For enemies that should face their target:

```cpp
void FollowerFacingSystem::updateTargetBased(Registry& registry, EntityId entity,
                                               FollowerFacingComponent& facing,
                                               const TransformComponent& transform)
{
    // Check if has follow AI with target
    if (!registry.hasComponent<FollowAIComponent>(entity)) return;
    
    auto& ai = registry.getComponent<FollowAIComponent>(entity);
    if (!registry.entityExists(ai.targetEntity)) return;
    
    auto& targetTransform = registry.getComponent<TransformComponent>(ai.targetEntity);
    
    // Face toward target
    bool shouldFaceRight = targetTransform.x > transform.x;
    
    if (!facing.initialized || facing.facingRight != shouldFaceRight) {
        facing.initialized = true;
        facing.facingRight = shouldFaceRight;
        
        auto& sprite = registry.getComponent<SpriteComponent>(entity);
        sprite.flipHorizontal = facing.facingRight;
    }
}
```

## Best Practices

1. **Initialize Properly**: Let first frame set initial direction
2. **Threshold Values**: Use reasonable thresholds (5-20 units)
3. **Hysteresis**: Add hysteresis for entities near stationary
4. **Consistent Assets**: All follower sprites should face same default direction
5. **Animation Sync**: Ensure animation timing continues through flips

## Performance Considerations

- **Sparse Component**: Only followers need this component
- **Simple Math**: Just velocity comparisons
- **Conditional Update**: Only updates when direction changes
- **No Allocations**: Pure data manipulation

## Related Components

- [FollowerFacingComponent](../components/follower-facing-component.md) - State data
- [VelocityComponent](../server/components/velocity-component.md) - Movement data
- [SpriteComponent](../components/sprite-component.md) - Visual output
- [AnimationComponent](../components/animation-component.md) - Animation state

## Related Systems

- [AnimationSystem](animation-system.md) - Frame animation
- [RenderSystem](render-system.md) - Sprite rendering
- [DirectionalAnimationSystem](directional-animation-system.md) - Related directional logic
