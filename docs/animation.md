# Animation System API

## SpriteComponent

ECS component for managing sprites with animation frame support.

### Properties

| Property | Type | Description |
|----------|------|-------------|
| `sprite` | `std::optional<sf::Sprite>` | SFML sprite (optional until setTexture) |
| `frameWidth` | `uint32_t` | Width of a frame in pixels |
| `frameHeight` | `uint32_t` | Height of a frame in pixels |
| `columns` | `uint32_t` | Number of columns in the spritesheet |
| `currentFrame` | `uint32_t` | Current frame index |

### Methods

#### `SpriteComponent()`
Default constructor. The sprite is not initialized.

#### `SpriteComponent(const sf::Texture& texture)`
Constructor with texture. Initializes the sprite with the full texture.

#### `void setTexture(const sf::Texture& texture)`
Sets the sprite texture.

#### `void setFrameSize(uint32_t width, uint32_t height, uint32_t cols = 1)`
Configures the frame size and number of columns in the spritesheet.

#### `void setFrame(uint32_t frameIndex)`
Changes the current frame. Automatically calculates UV coordinates:
- `x = (frameIndex % columns) * frameWidth`
- `y = (frameIndex / columns) * frameHeight`

#### `uint32_t getFrame() const`
Returns the current frame index.

#### `void setPosition(float x, float y)`
Sets the sprite position.

#### `void setScale(float x, float y)`
Sets the sprite scale.

#### `const sf::Sprite* raw() const`
Returns a pointer to the SFML sprite (nullptr if no texture).

#### `bool hasSprite() const`
Returns true if the sprite is initialized.

---

## AnimationComponent

ECS component for defining an animation (frames, timing, direction).

### Properties

| Property | Type | Description |
|----------|------|-------------|
| `frameIndices` | `vector<uint32_t>` | List of frame indices to play |
| `frameTime` | `float` | Duration of each frame in seconds |
| `elapsedTime` | `float` | Time elapsed since last frame change |
| `currentFrame` | `uint32_t` | Index in frameIndices |
| `loop` | `bool` | Whether the animation loops |
| `playing` | `bool` | Whether the animation is playing |
| `finished` | `bool` | Whether the animation has finished |
| `direction` | `AnimationDirection` | Forward, Reverse, or PingPong |

### Factory Methods

#### `static AnimationComponent create(uint32_t frameCount, float frameTime, bool loop = true)`
Creates a sequential animation (0, 1, 2, ..., frameCount-1).

```cpp
auto anim = AnimationComponent::create(8, 0.1F); // 8 frames at 100ms
```

#### `static AnimationComponent fromIndices(vector<uint32_t> indices, float frameTime, bool loop = true)`
Creates an animation with specific frames.

```cpp
auto walk = AnimationComponent::fromIndices({2, 3, 4, 3}, 0.1F);
```

### Control Methods

#### `void play()`
Resumes the animation.

#### `void pause()`
Pauses the animation.

#### `void stop()`
Stops and resets to frame 0.

#### `void reset()`
Resets to frame 0 without stopping.

#### `uint32_t getCurrentFrameIndex() const`
Returns the current frame index (value in frameIndices).

---

## AnimationSystem

ECS system that updates animations.

### Methods

#### `void update(Registry& registry, float deltaTime)`
Updates all entities with `AnimationComponent` and `SpriteComponent`.

---

## AnimationDirection

```cpp
enum class AnimationDirection : uint8_t {
    Forward,   // 0 → 1 → 2 → 3 → 0
    Reverse,   // 3 → 2 → 1 → 0 → 3
    PingPong   // 0 → 1 → 2 → 3 → 2 → 1 → 0
};
```

---

## Usage Example

```cpp
// Setup
Registry registry;
AnimationSystem animSystem;
TextureManager textures;

// Load texture
const auto& texture = textures.load("player", "player_spritesheet.png");

// Create entity
EntityId player = registry.createEntity();

// Add SpriteComponent
auto& sprite = registry.emplace<SpriteComponent>(player, texture);
sprite.setFrameSize(32, 32, 8);  // 8 frames of 32x32
sprite.setPosition(100, 100);

// Add AnimationComponent
auto& anim = registry.emplace<AnimationComponent>(player,
    AnimationComponent::create(8, 0.1F));
anim.direction = AnimationDirection::PingPong;

// Game loop
while (running) {
    float dt = clock.restart().asSeconds();
    animSystem.update(registry, dt);

    // Render
    if (sprite.hasSprite()) {
        window.draw(*sprite.raw());
    }
}
```
