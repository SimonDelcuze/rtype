# HUDSystem

**Location:** `client/include/systems/HUDSystem.hpp`

Renders HUD text for score and draws player lives. It reads shared gameplay data (`ScoreComponent`, `LivesComponent`) and client render data (`TextComponent`, `TransformComponent`), resolves fonts via `FontManager`, and draws on top of the scene.

---

## Responsibilities

- Format HUD strings:
  - Score: `Score: <value>`
- Apply render settings from `TextComponent`:
  - `fontId` → resolve `sf::Font` in `FontManager`
  - `characterSize`, `color`, `content`
  - Position/scale/rotation from `TransformComponent`
- Draw to the `Window` each frame.
- Render life pips for every player in the bottom-left HUD.

---

## Required Components

An entity must have:
- `TransformComponent` — screen position/scale/rotation for the text.
- `TextComponent` — font id, color, size, content, optional cached `sf::Text`.

Optional:
- `ScoreComponent` — if present, HUD shows `Score: value`.

Layering:
- Use `LayerComponent` with `RenderLayer::HUD` to ensure the HUD renders above game entities.

---

## Update Flow

1) Iterate entities with `TransformComponent` + `TextComponent`.
2) Skip dead entities.
3) Compute text content from `ScoreComponent`.
4) Resolve font via `FontManager::get(fontId)`; skip drawing if missing.
5) Create or update `sf::Text`:
   - `setFont`, `setString(content)`, `setCharacterSize`, `setFillColor`.
   - Apply transform: position, scale, rotation from `TransformComponent`.
6) `window.draw(sf::Text)`.
7) Collect player entities (`TagComponent` + `LivesComponent`), sort by `OwnershipComponent.ownerId`, and draw one lives row per player.

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

// Register system after render
gameLoop.addSystem(std::make_shared<RenderSystem>(window));
gameLoop.addSystem(std::make_shared<HUDSystem>(window, fontManager));
```

---

## Notes & Best Practices

- Ensure the font is loaded before drawing; missing fonts skip drawing but keep content updated.
- Keep HUD entities out of replication if they are client-only displays.
- Adjust pip size/spacing in `HUDSystem` if you want a different visual style.
