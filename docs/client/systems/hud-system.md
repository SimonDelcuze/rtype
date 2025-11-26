# HUDSystem

**Location:** `client/include/systems/HUDSystem.hpp`

Renders HUD text for score and lives. It reads shared gameplay data (`ScoreComponent`, `LivesComponent`) and client render data (`TextComponent`, `TransformComponent`), resolves fonts via `FontManager`, and draws SFML text on top of the scene.

---

## Responsibilities

- Format HUD strings:
  - Score: `Score: <value>`
  - Lives: `Lives: <current>/<max>`
- Apply render settings from `TextComponent`:
  - `fontId` → resolve `sf::Font` in `FontManager`
  - `characterSize`, `color`, `content`
  - Position/scale/rotation from `TransformComponent`
- Draw to the `Window` each frame.
- Render simple life pips (small rectangles) beneath lives text.

---

## Required Components

An entity must have:
- `TransformComponent` — screen position/scale/rotation for the text.
- `TextComponent` — font id, color, size, content, optional cached `sf::Text`.

Optional (mutually exclusive priority):
- `ScoreComponent` — if present, HUD shows `Score: value`.
- `LivesComponent` — if no score is present, HUD shows `Lives: current/max` and draws pips.

Layering:
- Use `LayerComponent` with `RenderLayer::HUD` to ensure the HUD renders above game entities.

---

## Update Flow

1) Iterate entities with `TransformComponent` + `TextComponent`.
2) Skip dead entities.
3) Compute text content from `ScoreComponent` or `LivesComponent` (score takes priority if both exist).
4) Resolve font via `FontManager::get(fontId)`; skip drawing if missing.
5) Create or update `sf::Text`:
   - `setFont`, `setString(content)`, `setCharacterSize`, `setFillColor`.
   - Apply transform: position, scale, rotation from `TransformComponent`.
6) `window.draw(sf::Text)`.
7) If `LivesComponent` present, draw pips rectangles under the text to visualize remaining lives.

---

## Usage Example

```cpp
// Load font once
fontManager.load("main", "client/assets/fonts/MainFont.ttf");

// Score HUD entity
EntityId scoreHud = registry.createEntity();
registry.emplace<TransformComponent>(scoreHud, TransformComponent::create(20.f, 20.f));
registry.emplace<LayerComponent>(scoreHud, LayerComponent::create(RenderLayer::HUD));
registry.emplace<TextComponent>(scoreHud, TextComponent::create("main", 24, sf::Color::White));
registry.emplace<ScoreComponent>(scoreHud, ScoreComponent::create(0));

// Lives HUD entity
EntityId livesHud = registry.createEntity();
registry.emplace<TransformComponent>(livesHud, TransformComponent::create(20.f, 60.f));
registry.emplace<LayerComponent>(livesHud, LayerComponent::create(RenderLayer::HUD));
registry.emplace<TextComponent>(livesHud, TextComponent::create("main", 24, sf::Color::White));
registry.emplace<LivesComponent>(livesHud, LivesComponent::create(3, 3));

// Register system after render
gameLoop.addSystem(std::make_shared<RenderSystem>(window));
gameLoop.addSystem(std::make_shared<HUDSystem>(window, fontManager));
```

---

## Notes & Best Practices

- Ensure the font is loaded before drawing; missing fonts skip drawing but keep content updated.
- Keep HUD entities out of replication if they are client-only displays.
- Score takes precedence when both score and lives components are present on the same entity; separate entities are recommended for clarity.
- Adjust pip size/spacing in `HUDSystem` if you want a different visual style.
