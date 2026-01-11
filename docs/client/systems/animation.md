# AnimationSystem

**Location:** `client/src/systems/AnimationSystem.cpp`

The `AnimationSystem` manages sprite sheet animations for all entities with `AnimationComponent` and `SpriteComponent`. It advances animation frames based on elapsed time and updates sprite texture rectangles automatically.

---

## **Purpose**

* **Advance animation frames** based on `frameTime` and `deltaTime`
* **Update sprite texture rects** to display the correct frame
* **Handle playback modes** (Forward, Reverse, PingPong)
* **Manage animation state** (playing, paused, finished)

---

## **System Signature**

```cpp
class AnimationSystem {
public:
    void update(Registry& registry, float deltaTime);

private:
    void advanceFrame(AnimationComponent& anim);
};
```

---

## **Required Components**

The AnimationSystem operates on entities with:

* **`AnimationComponent`** — Animation state and configuration
* **`SpriteComponent`** — Sprite to update with new frames

---

## **Update Flow**

### Main Loop

```cpp
void AnimationSystem::update(Registry& registry, float deltaTime) {
    for (EntityId entity : registry.view<AnimationComponent, SpriteComponent>()) {
        auto& anim = registry.get<AnimationComponent>(entity);
        auto& sprite = registry.get<SpriteComponent>(entity);
        
        // Skip if not playing or already finished
        if (!anim.playing || anim.finished || anim.frameIndices.empty()) {
            continue;
        }
        
        // Accumulate time
        anim.elapsedTime += deltaTime;
        
        // Advance frame when enough time has passed
        if (anim.elapsedTime >= anim.frameTime) {
            anim.elapsedTime -= anim.frameTime;  // Carry over excess
            advanceFrame(anim);
            
            // Update sprite texture rect
            sprite.setFrame(anim.getCurrentFrameIndex());
        }
    }
}
```

---

## **Frame Advancement Logic**

### Forward Direction

```cpp
anim.currentFrame++;

if (anim.currentFrame >= frameCount) {
    if (anim.loop) {
        anim.currentFrame = 0;  // Restart
    } else {
        anim.currentFrame = frameCount - 1;  // Hold last frame
        anim.finished = true;
        anim.playing = false;
    }
}
```

### Reverse Direction

```cpp
if (anim.currentFrame == 0) {
    if (anim.loop) {
        anim.currentFrame = frameCount - 1;  // Wrap to end
    } else {
        anim.finished = true;
        anim.playing = false;
    }
} else {
    anim.currentFrame--;
}
```

### PingPong Direction

Bounces back and forth between first and last frame:

```cpp
if (anim.pingPongReverse) {
    // Going backwards
    if (anim.currentFrame == 0) {
        anim.pingPongReverse = false;  // Change direction
        if (!anim.loop) {
            anim.finished = true;
            anim.playing = false;
        } else {
            anim.currentFrame++;  // Start going forward
        }
    } else {
        anim.currentFrame--;
    }
} else {
    // Going forwards
    anim.currentFrame++;
    if (anim.currentFrame >= frameCount - 1) {
        anim.pingPongReverse = true;  // Change direction
    }
}
```

---

## **Integration with Rendering**

The AnimationSystem updates component data; the RenderSystem displays sprites:

```cpp
// Render System
for (EntityId id : registry.view<Position, SpriteComponent>()) {
    auto& pos = registry.get<Position>(id);
    auto& sprite =registry.get<SpriteComponent>(id);
    
    sprite.setPosition(pos.x, pos.y);
    window.draw(*sprite.raw());
}
```

**Flow:**
1. AnimationSystem updates `SpriteComponent::currentFrame`
2. `sprite.setFrame()` updates the texture rectangle
3. RenderSystem draws the sprite with the correct frame

---

## **Example: Walking Player**

```cpp
Registry registry;
EntityId player = registry.createEntity();

// Setup sprite
sf::Texture playerTexture;
playerTexture.loadFromFile("assets/player.png");

SpriteComponent sprite(playerTexture);
sprite.setFrameSize(32, 32, 8);  // 32x32 frames, 8 columns

registry.emplace<SpriteComponent>(player, sprite);

// Setup animation (4-frame walk cycle at 12 FPS)
auto walkAnim = AnimationComponent::create(4, 1.0f / 12.0f, true);
walkAnim.direction = AnimationDirection::Forward;

registry.emplace<AnimationComponent>(player, walkAnim);

// AnimationSystem will automatically advance frames
```

**Result:** Player sprite cycles through frames 0, 1, 2, 3, 0, 1, 2, 3...

---

## **Example: One-Shot Explosion**

```cpp
EntityId explosion = registry.createEntity();

// Non-looping explosion
auto explosionAnim = AnimationComponent::create(8, 0.05f, false);
registry.emplace<AnimationComponent>(explosion, explosionAnim);

// Sprite setup...
registry.emplace<SpriteComponent>(explosion, explosionSprite);

// In game loop, destroy when finished
if (registry.get<AnimationComponent>(explosion).finished) {
    registry.destroyEntity(explosion);
}
```

**Result:** Explosion plays once, then entity is destroyed.

---

## **Performance Considerations**

### Optimizations

* **Early exit** — Skips paused/finished animations
* **Frame time accumulation** — Handles variable frame rates smoothly
* **Direct sprite update** — No intermediate buffers

### Frame Rate Independence

The system uses **delta time accumulation**:

```cpp
elapsedTime += deltaTime;

while (elapsedTime >= frameTime) {
    elapsedTime -= frameTime;
    advanceFrame();
}
```

This ensures animations play at consistent speed regardless of FPS.

---

## **State Diagram**

```
[Playing] ---(pause)---> [Paused]
    |
    |---(time >= frameTime)---> [Advance Frame]
    |
    |---(currentFrame == last && !loop)---> [Finished]

[Paused] ---(play)---> [Playing]
[Finished] ---(reset)---> [Playing]
```

---

## **Edge Cases Handled**

### Empty Frame Indices

```cpp
if (anim.frameIndices.empty()) {
    return;  // Skip animation
}
```

### Non-Looping Completion

```cpp
if (!anim.loop && anim.currentFrame >= frameCount) {
    anim.finished = true;
    anim.playing = false;
    anim.currentFrame = frameCount - 1;  // Clamp to last frame
}
```

### Time Overflow

```cpp
// Handle cases where deltaTime > frameTime
while (anim.elapsedTime >= anim.frameTime) {
    anim.elapsedTime -= anim.frameTime;
    advanceFrame(anim);
}
```

---

## **Testing**

See `tests/client/animation/AnimationSystemTests.cpp`:

```cpp
TEST(AnimationSystem, AdvancesFrameWhenTimeExceeds) {
    Registry registry;
    AnimationSystem system;
    
    EntityId entity = registry.createEntity();
    auto anim = AnimationComponent::create(4, 0.1f, true);
    registry.emplace<AnimationComponent>(entity, anim);
    
    system.update(registry, 0.05f);  // Half frame time
    EXPECT_EQ(registry.get<AnimationComponent>(entity).currentFrame, 0);
    
    system.update(registry, 0.06f);  // Total 0.11s > 0.1s
    EXPECT_EQ(registry.get<AnimationComponent>(entity).currentFrame, 1);
}
```

---

## **Common Patterns**

### Start/Stop Animation

```cpp
auto& anim = registry.get<AnimationComponent>(player);

// Start
anim.play();

// Stop and reset
anim.stop();
```

### Change Animation Speed

```cpp
auto& anim = registry.get<AnimationComponent>(player);

// Slow motion (2x slower)
anim.frameTime = 0.2f;

// Fast forward (2x faster)
anim.frameTime = 0.05f;
```

### Swap Animation

```cpp
// Remove old animation
registry.remove<AnimationComponent>(entity);

// Add new one
auto newAnim = AnimationComponent::fromIndices({0, 1, 2, 3}, 0.08f, true);
registry.emplace<AnimationComponent>(entity, newAnim);
```

---

## **Related**

* [AnimationComponent](../components/animation-component.md) — Data structure for animations
* [SpriteComponent](../components/sprite-component.md) — Sprite rendering
* [RenderSystem](TODO) — Draws sprites to screen
* [Delta Time](TODO) — Frame timing explanation
