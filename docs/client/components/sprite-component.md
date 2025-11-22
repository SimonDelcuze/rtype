# SpriteComponent

**Location:** `client/include/components/SpriteComponent.hpp`

The `SpriteComponent` wraps `sf::Sprite` from SFML and provides utilities for rendering entities with sprite sheets, including frame selection and transformations.

---

## **Structure**

```cpp
struct SpriteComponent {
    std::optional<sf::Sprite> sprite;
    std::uint32_t frameWidth   = 0;
    std::uint32_t frameHeight  = 0;
    std::uint32_t columns      = 1;
    std::uint32_t currentFrame = 0;
    
    SpriteComponent() = default;
    explicit SpriteComponent(const sf::Texture& texture);
    
    void setTexture(const sf::Texture& texture);
    void setPosition(float x, float y);
    void setScale(float x, float y);
    void setFrame(std::uint32_t frameIndex);
    void setFrameSize(std::uint32_t width, std::uint32_t height, std::uint32_t cols = 1);
    std::uint32_t getFrame() const;
    const sf::Sprite* raw() const;
    bool hasSprite() const;
};
```

---

## **Fields**

| Field | Type | Description |
|-------|------|-------------|
| `sprite` | `std::optional<sf::Sprite>` | SFML sprite object (optional for lazy init) |
| `frameWidth` | `uint32_t` | Width of each frame in sprite sheet |
| `frameHeight` | `uint32_t` | Height of each frame in sprite sheet |
| `columns` | `uint32_t` | Number of frames per row in sprite sheet |
| `currentFrame` | `uint32_t` | Current frame index being displayed |

---

## **Constructors**

### Default Constructor

```cpp
SpriteComponent() = default;
```

Creates an empty sprite component (no texture).

---

### Texture Constructor

```cpp
explicit SpriteComponent(const sf::Texture& texture);
```

Creates a sprite with the given texture.

**Example:**
```cpp
sf::Texture playerTexture;
playerTexture.loadFromFile("assets/player.png");

SpriteComponent spr(playerTexture);
registry.emplace<SpriteComponent>(player, spr);
```

---

## **Methods**

### Set Texture

```cpp
void setTexture(const sf::Texture& texture);
```

Assigns a texture to the sprite.

**Example:**
```cpp
auto& spr = registry.get<SpriteComponent>(enemy);
spr.setTexture(enemyTexture);
```

---

### Set Position

```cpp
void setPosition(float x, float y);
```

Sets the sprite's screen position.

**Example:**
```cpp
auto& pos = registry.get<Position>(player);
auto& spr = registry.get<SpriteComponent>(player);
spr.setPosition(pos.x, pos.y);
```

---

### Set Scale

```cpp
void setScale(float x, float y);
```

Scales the sprite (e.g., 2.0 for double size).

**Example:**
```cpp
auto& spr = registry.get<SpriteComponent>(boss);
spr.setScale(3.0f, 3.0f);  // 3x larger
```

---

### Set Frame

```cpp
void setFrame(std::uint32_t frameIndex);
```

Selects a frame from the sprite sheet by index.

**How it works:**
1. Calculates row: `row = frameIndex / columns`
2. Calculates column: `col = frameIndex % columns`  
3. Sets texture rect: `IntRect(col * frameWidth, row * frameHeight, frameWidth, frameHeight)`

**Example:**
```cpp
auto& spr = registry.get<SpriteComponent>(player);
spr.setFrameSize(32, 32, 8);  // 32x32 frames, 8 per row
spr.setFrame(5);  // Display 6th frame (row 0, col 5)
```

---

### Set Frame Size

```cpp
void setFrameSize(std::uint32_t width, std::uint32_t height, std::uint32_t cols = 1);
```

Configures sprite sheet parameters.

**Parameters:**
* `width` — Width of each frame
* `height` — Height of each frame
* `cols` — Frames per row (default: 1)

**Example:**
```cpp
// Sprite sheet: 64x64 frames, 4 columns
auto& spr = registry.get<SpriteComponent>(monster);
spr.setFrameSize(64, 64, 4);
```

---

### Get Current Frame

```cpp
std::uint32_t getFrame() const;
```

Returns the currently displayed frame index.

**Example:**
```cpp
auto& spr = registry.get<SpriteComponent>(player);
uint32_t frame = spr.getFrame();  // e.g., 3
```

---

### Get Raw Sprite Pointer

```cpp
const sf::Sprite* raw() const;
```

Returns a pointer to the underlying `sf::Sprite` for SFML rendering.

**Example:**
```cpp
auto& spr = registry.get<SpriteComponent>(entity);
if (spr.hasSprite()) {
    window.draw(*spr.raw());
}
```

---

### Check if Sprite Exists

```cpp
bool hasSprite() const;
```

Returns `true` if the sprite has been initialized with a texture.

**Example:**
```cpp
auto& spr = registry.get<SpriteComponent>(entity);
if (!spr.hasSprite()) {
    spr.setTexture(defaultTexture);
}
```

---

## **Sprite Sheet Layout**

Sprite sheets are organized in a grid:

```
Frame 0  | Frame 1  | Frame 2  | Frame 3
---------+----------+----------+---------
Frame 4  | Frame 5  | Frame 6  | Frame 7
---------+----------+----------+---------
Frame 8  | Frame 9  | Frame 10 | Frame 11
```

**Example:** For a 128x128 sprite sheet with 32x32 frames:
* `columns = 4` (4 frames per row)
* `frameWidth = 32`
* `frameHeight = 32`
* Frame 5 is at position (32, 32) in the texture

---

## **Usage in Rendering System**

```cpp
// RenderSystem::render (simplified)
sf::RenderWindow window;

for (EntityId id : registry.view<Position, SpriteComponent>()) {
    auto& pos = registry.get<Position>(id);
    auto& spr = registry.get<SpriteComponent>(id);
    
    if (!spr.hasSprite()) continue;
    
    spr.setPosition(pos.x, pos.y);
    window.draw(*spr.raw());
}
```

---

## **Integration with AnimationComponent**

The `AnimationSystem` automatically updates `SpriteComponent` frames:

```cpp
// AnimationSystem updates frame
for (EntityId id : registry.view<AnimationComponent, SpriteComponent>()) {
    auto& anim = registry.get<AnimationComponent>(id);
    auto& sprite = registry.get<SpriteComponent>(id);
    
    // When animation advances...
    sprite.setFrame(anim.getCurrentFrameIndex());
}
```

---

## **Example: Animated Player**

```cpp
Registry registry;
EntityId player = registry.createEntity();

// Load texture
sf::Texture playerTexture;
playerTexture.loadFromFile("assets/player_spritesheet.png");

// Create sprite component
SpriteComponent spr(playerTexture);
spr.setFrameSize(32, 32, 8);  // 32x32 frames, 8 columns
spr.setFrame(0);  // Start at first frame

registry.emplace<SpriteComponent>(player, spr);

// Animation will control frame changes
auto anim = AnimationComponent::create(8, 0.1f, true);
registry.emplace<AnimationComponent>(player, anim);
```

---

## **Performance Considerations**

* **`std::optional<sf::Sprite>`** — Allows lazy initialization, saves memory for non-rendered entities
* **Shared textures** — Multiple sprites can reference the same `sf::Texture`
* **Frame calculation** — Simple integer division/modulo, very fast

---

## **Common Patterns**

### Static Sprite (No Animation)

```cpp
SpriteComponent spr(backgroundTexture);
spr.setPosition(0, 0);
// No setFrame calls, displays full texture
```

### Single-Row Sprite Sheet

```cpp
SpriteComponent spr(explosionTexture);
spr.setFrameSize(64, 64, 8);  // 8 frames in one row
// frames 0-7 are in row 0
```

### Multi-Row Sprite Sheet

```cpp
SpriteComponent spr(enemyTexture);
spr.setFrameSize(48, 48, 4);  // 4 columns
spr.setFrame(6);  // Row 1, column 2
```

---

## **Related**

* [AnimationComponent](animation-component.md) — Controls sprite frame sequences
* [Position Component](TODO) — Determines sprite screen position
* [RenderSystem](TODO) — Draws sprites to screen
* [SFML Sprite Documentation](https://www.sfml-dev.org/documentation/2.6.1/classsf_1_1Sprite.php)
