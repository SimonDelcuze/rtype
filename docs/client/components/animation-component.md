# AnimationComponent

**Location:** `client/include/components/AnimationComponent.hpp`

The `AnimationComponent` manages sprite sheet animations for entities on the client side. It supports multiple playback modes, frame sequences, and control methods.

---

## **Structure**

```cpp
struct AnimationComponent {
    std::vector<std::uint32_t> frameIndices;
    float frameTime              = 0.1F;
    float elapsedTime            = 0.0F;
    std::uint32_t currentFrame   = 0;
    bool loop                    = true;
    bool playing                 = true;
    bool finished                = false;
    AnimationDirection direction = AnimationDirection::Forward;
    bool pingPongReverse         = false;
    
    static AnimationComponent create(std::uint32_t frameCount, float frameTime, bool loop = true);
    static AnimationComponent fromIndices(std::vector<std::uint32_t> indices, float frameTime, bool loop = true);
    
    void play();
    void pause();
    void stop();
    void reset();
    std::uint32_t getCurrentFrameIndex() const;
};
```

---

## **Animation Direction**

```cpp
enum class AnimationDirection : std::uint8_t {
    Forward,   // 0 → 1 → 2 → ...
    Reverse,   // ... → 2 → 1 → 0
    PingPong   // 0 → 1 → 2 → 1 → 0 (back and forth)
};
```

---

## **Fields**

| Field | Type | Description |
|-------|------|-------------|
| `frameIndices` | `std::vector<uint32_t>` | Sequence of frame indices to play |
| `frameTime` | `float` | Duration of each frame in seconds (e.g., 0.1s = 10 FPS) |
| `elapsedTime` | `float` | Time accumulator for current frame |
| `currentFrame` | `uint32_t` | Current position in `frameIndices` |
| `loop` | `bool` | Whether animation repeats |
| `playing` | `bool` | Whether animation is currently playing |
| `finished` | `bool` | Whether non-looping animation has completed |
| `direction` | `AnimationDirection` | Playback direction |
| `pingPongReverse` | `bool` | Internal state for PingPong mode |

---

## Factory Methods**

### Sequential Frames

```cpp
static AnimationComponent create(std::uint32_t frameCount, float frameTime, bool loop = true);
```

Creates an animation with sequential frames (0, 1, 2, ..., frameCount-1).

**Parameters:**
* `frameCount` — Number of frames in the animation
* `frameTime` — Duration per frame (seconds)
* `loop` — Whether to repeat (default: true)

**Example:**
```cpp
// 8-frame animation at 10 FPS, looping
auto anim = AnimationComponent::create(8, 0.1f, true);
registry.emplace<AnimationComponent>(entity, anim);
```

---

### Custom Frame Sequence

```cpp
static AnimationComponent fromIndices(std::vector<std::uint32_t> indices, float frameTime, bool loop = true);
```

Creates an animation with a custom frame order.

**Parameters:**
* `indices` — Specific frame sequence (e.g., {0, 1, 2, 1} for a bounce)
* `frameTime` — Duration per frame (seconds)
* `loop` — Whether to repeat (default: true)

**Example:**
```cpp
// Custom sequence: idle bounce effect
auto anim = AnimationComponent::fromIndices({0, 1, 2, 1, 0}, 0.15f, true);
registry.emplace<AnimationComponent>(entity, anim);
```

---

## **Control Methods**

### Play

```cpp
void play();
```

Starts or resumes the animation.

**Example:**
```cpp
auto& anim = registry.get<AnimationComponent>(player);
anim.play();
```

---

### Pause

```cpp
void pause();
```

Pauses the animation at the current frame.

**Example:**
```cpp
auto& anim = registry.get<AnimationComponent>(player);
anim.pause();
```

---

### Stop

```cpp
void stop();
```

Stops the animation and resets to the first frame.

**Example:**
```cpp
auto& anim = registry.get<AnimationComponent>(player);
anim.stop();
```

---

### Reset

```cpp
void reset();
```

Resets the animation to the beginning without stopping playback.

**Example:**
```cpp
auto& anim = registry.get<AnimationComponent>(player);
anim.reset();  // Restart from frame 0
```

---

### Get Current Frame Index

```cpp
std::uint32_t getCurrentFrameIndex() const;
```

Returns the current frame index from `frameIndices`.

**Example:**
```cpp
auto& anim = registry.get<AnimationComponent>(player);
uint32_t frame = anim.getCurrentFrameIndex();  // e.g., 3
```

---

## **Usage with AnimationSystem**

The `AnimationSystem` updates all entities with `AnimationComponent` and `SpriteComponent`:

```cpp
// AnimationSystem::update (simplified)
for (EntityId id : registry.view<AnimationComponent, SpriteComponent>()) {
    auto& anim = registry.get<AnimationComponent>(id);
    auto& sprite = registry.get<SpriteComponent>(id);
    
    if (!anim.playing || anim.finished) continue;
    
    anim.elapsedTime += deltaTime;
    
    if (anim.elapsedTime >= anim.frameTime) {
        anim.elapsedTime = 0.0f;
        advanceFrame(anim);  // Move to next frame
        
        // Update sprite texture rect
        sprite.setFrame(anim.getCurrentFrameIndex());
    }
}
```

---

## **Playback Modes**

### Forward (Default)

Frames play from start to end:
```
0 → 1 → 2 → 3 → [loop to 0] or [stop]
```

### Reverse

Frames play from end to start:
```
3 → 2 → 1 → 0 → [loop to 3] or [stop]
```

### PingPong

Frames bounce back and forth:
```
0 → 1 → 2 → 3 → 2 → 1 → 0 → [repeat]
```

---

## **Example: Player Walk Animation**

```cpp
Registry registry;
EntityId player = registry.createEntity();

// Create 4-frame walk cycle at 12 FPS
auto walkAnim = AnimationComponent::create(4, 1.0f / 12.0f, true);
walkAnim.direction = AnimationDirection::Forward;

registry.emplace<AnimationComponent>(player, walkAnim);
registry.emplace<SpriteComponent>(player, playerTexture);

// AnimationSystem will automatically advance frames
```

---

## **Example: One-Shot Explosion**

```cpp
// Create non-looping explosion animation
auto explosionAnim = AnimationComponent::create(8, 0.05f, false);

registry.emplace<AnimationComponent>(explosion, explosionAnim);

// When finished == true, destroy the entity
if (registry.get<AnimationComponent>(explosion).finished) {
    registry.destroyEntity(explosion);
}
```

---

## **Performance Considerations**

* **Frame indices vector** — Allows complex sequences without duplicating sprite data
* **Shared sprite sheets** — Multiple entities can use the same texture
* **Delta time accumulation** — Smooth frame transitions regardless of FPS

---

## **Testing**

See `tests/client/animation/AnimationComponentTests.cpp`:

```cpp
TEST(AnimationComponent, CreateWithFrameCount) {
    auto anim = AnimationComponent::create(8, 0.1F, true);
    
    EXPECT_EQ(anim.frameIndices.size(), 8);
    EXPECT_FLOAT_EQ(anim.frameTime, 0.1F);
    EXPECT_TRUE(anim.loop);
    EXPECT_TRUE(anim.playing);
    EXPECT_FALSE(anim.finished);
    EXPECT_EQ(anim.currentFrame, 0);
    
    for (uint32_t i = 0; i < 8; ++i) {
        EXPECT_EQ(anim.frameIndices[i], i);
    }
}
```

---

## **Related**

* [SpriteComponent](sprite-component.md) — Required for rendering animated sprites
* [AnimationSystem](../../client/systems/animation-system.md) — Processes this component
* [Sprite Sheets](TODO) — Asset organization for animations
