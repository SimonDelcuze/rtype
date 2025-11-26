# TextComponent

**Location:** `client/include/components/TextComponent.hpp`

Client-only component that carries text rendering data for HUD/UI elements. Pairs with `FontManager` to resolve fonts by ID and with `HUDSystem` to display score/lives.

---

## Structure

```cpp
struct TextComponent
{
    std::string fontId;
    unsigned int characterSize = 24;
    sf::Color color            = sf::Color::White;
    std::string content;
    std::optional<sf::Text> text;

    static TextComponent create(const std::string& font, unsigned int size, const sf::Color& c);
};
```

---

## Behavior

- `fontId` references a font loaded in `FontManager` (not an ownership of `sf::Font`).
- `content` holds the raw string to display; `HUDSystem` updates it from game state.
- `text` is an optional cached `sf::Text` to avoid recreating every frame. It is populated when the system finds a font.
- `characterSize` and `color` are applied each update before drawing.
- `resetValue<TextComponent>` clears the optional and resets fields to defaults.

---

## Usage

```cpp
// Create a HUD text entity
EntityId hud = registry.createEntity();
registry.emplace<TransformComponent>(hud, TransformComponent::create(20.f, 20.f));
registry.emplace<LayerComponent>(hud, LayerComponent::create(RenderLayer::HUD));
registry.emplace<TextComponent>(hud, TextComponent::create("main", 24, sf::Color::White));

// Set initial content; HUDSystem will also override based on score/lives components
registry.get<TextComponent>(hud).content = "Score: 0";
```

---

## Notes

- Purely client-side; do not replicate.
- Keep `fontId` stable and ensure the corresponding font is loaded before drawing (`FontManager::load`).
- Use alongside `HUDSystem` to automate content updates from `ScoreComponent`/`LivesComponent`.
