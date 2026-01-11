# GameOverMenu

## Overview

The `GameOverMenu` displays post-game results including final score, statistics, and offers players options to return to lobby, view detailed stats, or exit. It shows whether the player won or lost and provides a summary of their performance.

## Class Structure

```cpp
class GameOverMenu : public IMenu
{
  public:
    struct Result
    {
        bool returnToLobby = false;
        bool exitRequested = false;
        bool viewStats     = false;
    };

    struct GameOverData
    {
        bool victory;
        std::uint32_t finalScore;
        float playTime;
        std::uint32_t enemiesDefeated;
        std::uint32_t bossesDefeated;
        std::uint32_t highestCombo;
        std::string playerName;
    };

    GameOverMenu(FontManager& fonts, TextureManager& textures, 
                 const GameOverData& data);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void render(Registry& registry, Window& window) override;

    Result getResult() const;

  private:
    FontManager& fonts_;
    TextureManager& textures_;
    GameOverData data_;
    Result result_;
    bool done_ = false;
};
```

## UI Layout - Victory

```
┌────────────────────────────────────────────────┐
│                                                │
│            ★ MISSION COMPLETE ★                │
│                                                │
│              FINAL SCORE: 125,400              │
│                                                │
│  ┌──────────────────────────────────────────┐ │
│  │  Enemies Defeated:       247             │ │
│  │  Bosses Defeated:        3               │ │
│  │  Highest Combo:          x42             │ │
│  │  Time:                   8:34            │ │
│  └──────────────────────────────────────────┘ │
│                                                │
│     [ Return to Lobby ]    [ View Stats ]     │
│                                                │
│                    [ Exit ]                    │
└────────────────────────────────────────────────┘
```

## UI Layout - Defeat

```
┌────────────────────────────────────────────────┐
│                                                │
│                  GAME OVER                     │
│                                                │
│              FINAL SCORE: 42,100               │
│                                                │
│  ┌──────────────────────────────────────────┐ │
│  │  Enemies Defeated:       89              │ │
│  │  Bosses Defeated:        1               │ │
│  │  Highest Combo:          x18             │ │
│  │  Time:                   4:12            │ │
│  └──────────────────────────────────────────┘ │
│                                                │
│     [ Return to Lobby ]    [ View Stats ]     │
│                                                │
│                    [ Exit ]                    │
└────────────────────────────────────────────────┘
```

## Menu Creation

```cpp
void GameOverMenu::create(Registry& registry)
{
    const float centerX = SCREEN_WIDTH / 2;
    const float startY = 100;
    
    // Title - changes based on victory/defeat
    auto title = registry.createEntity();
    registry.addComponent(title, TransformComponent::create(centerX, startY));
    
    std::string titleText;
    Color titleColor;
    
    if (data_.victory) {
        titleText = "★ MISSION COMPLETE ★";
        titleColor = Color{100, 255, 100};  // Green
    } else {
        titleText = "GAME OVER";
        titleColor = Color{255, 100, 100};  // Red
    }
    
    registry.addComponent(title, TextComponent::create(titleText, 42, titleColor));
    registry.addComponent(title, LayerComponent::create(RenderLayer::UI));
    
    // Final score
    auto scoreText = registry.createEntity();
    registry.addComponent(scoreText, TransformComponent::create(centerX, startY + 60));
    registry.addComponent(scoreText, TextComponent::create(
        "FINAL SCORE: " + formatScore(data_.finalScore), 
        32, Color{255, 215, 0}  // Gold
    ));
    registry.addComponent(scoreText, LayerComponent::create(RenderLayer::UI));
    
    // Stats box background
    auto statsBox = registry.createEntity();
    registry.addComponent(statsBox, TransformComponent::create(centerX - 200, startY + 130));
    registry.addComponent(statsBox, BoxComponent::create(400, 160,
        Color{30, 30, 40, 200}, Color{70, 70, 90}));
    registry.addComponent(statsBox, LayerComponent::create(RenderLayer::UI));
    
    // Individual stats
    float statY = startY + 150;
    const float statSpacing = 30;
    
    createStatLine(registry, "Enemies Defeated:", 
                   std::to_string(data_.enemiesDefeated), centerX, statY);
    statY += statSpacing;
    
    createStatLine(registry, "Bosses Defeated:", 
                   std::to_string(data_.bossesDefeated), centerX, statY);
    statY += statSpacing;
    
    createStatLine(registry, "Highest Combo:", 
                   "x" + std::to_string(data_.highestCombo), centerX, statY);
    statY += statSpacing;
    
    createStatLine(registry, "Time:", 
                   formatTime(data_.playTime), centerX, statY);
    
    // Buttons
    const float btnY = startY + 330;
    
    // Return to lobby button
    auto lobbyBtn = registry.createEntity();
    registry.addComponent(lobbyBtn, TransformComponent::create(centerX - 180, btnY));
    registry.addComponent(lobbyBtn, BoxComponent::create(160, 50,
        Color{50, 100, 180}, Color{80, 140, 220}));
    registry.addComponent(lobbyBtn, ButtonComponent::create("Return to Lobby", [this]() {
        result_.returnToLobby = true;
        done_ = true;
    }));
    registry.addComponent(lobbyBtn, TextComponent::create("Return to Lobby", 16));
    registry.addComponent(lobbyBtn, FocusableComponent::create(0));
    registry.addComponent(lobbyBtn, LayerComponent::create(RenderLayer::UI));
    
    // View stats button
    auto statsBtn = registry.createEntity();
    registry.addComponent(statsBtn, TransformComponent::create(centerX + 20, btnY));
    registry.addComponent(statsBtn, BoxComponent::create(160, 50,
        Color{50, 120, 80}, Color{80, 160, 120}));
    registry.addComponent(statsBtn, ButtonComponent::create("View Stats", [this]() {
        result_.viewStats = true;
        done_ = true;
    }));
    registry.addComponent(statsBtn, TextComponent::create("View Stats", 16));
    registry.addComponent(statsBtn, FocusableComponent::create(1));
    registry.addComponent(statsBtn, LayerComponent::create(RenderLayer::UI));
    
    // Exit button
    auto exitBtn = registry.createEntity();
    registry.addComponent(exitBtn, TransformComponent::create(centerX - 60, btnY + 70));
    registry.addComponent(exitBtn, BoxComponent::create(120, 45,
        Color{70, 50, 50}, Color{120, 80, 80}));
    registry.addComponent(exitBtn, ButtonComponent::create("Exit", [this]() {
        result_.exitRequested = true;
        done_ = true;
    }));
    registry.addComponent(exitBtn, TextComponent::create("Exit", 16));
    registry.addComponent(exitBtn, FocusableComponent::create(2));
    registry.addComponent(exitBtn, LayerComponent::create(RenderLayer::UI));
}

void GameOverMenu::createStatLine(Registry& registry, const std::string& label,
                                    const std::string& value, float centerX, float y)
{
    // Label (left-aligned)
    auto labelEntity = registry.createEntity();
    registry.addComponent(labelEntity, TransformComponent::create(centerX - 150, y));
    registry.addComponent(labelEntity, TextComponent::create(label, 16, Color{200, 200, 220}));
    registry.addComponent(labelEntity, LayerComponent::create(RenderLayer::UI));
    
    // Value (right-aligned)
    auto valueEntity = registry.createEntity();
    registry.addComponent(valueEntity, TransformComponent::create(centerX + 50, y));
    registry.addComponent(valueEntity, TextComponent::create(value, 16, Color{255, 255, 255}));
    registry.addComponent(valueEntity, LayerComponent::create(RenderLayer::UI));
}
```

## Formatting Utilities

```cpp
std::string GameOverMenu::formatScore(std::uint32_t score)
{
    std::string str = std::to_string(score);
    std::string result;
    
    int count = 0;
    for (auto it = str.rbegin(); it != str.rend(); ++it) {
        if (count > 0 && count % 3 == 0) {
            result = ',' + result;
        }
        result = *it + result;
        count++;
    }
    
    return result;
}

std::string GameOverMenu::formatTime(float seconds)
{
    int minutes = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    
    std::stringstream ss;
    ss << minutes << ':';
    if (secs < 10) ss << '0';
    ss << secs;
    
    return ss.str();
}
```

## Victory Animation (Optional)

```cpp
void GameOverMenu::render(Registry& registry, Window& window)
{
    // Base rendering handled by RenderSystem
    
    if (data_.victory) {
        // Add particle effects or animation for victory
        static float animTime = 0.0F;
        animTime += 0.016F;  // ~60 FPS
        
        // Pulsating glow on score
        float pulse = 0.8F + 0.2F * std::sin(animTime * 3.0F);
        // Apply to score entity alpha or scale
    }
}
```

## Sending Results to Server

```cpp
void GameOverMenu::sendResultsToServer()
{
    GameResultPacket packet;
    packet.victory = data_.victory;
    packet.finalScore = data_.finalScore;
    packet.playTime = data_.playTime;
    packet.enemiesDefeated = data_.enemiesDefeated;
    packet.bossesDefeated = data_.bossesDefeated;
    packet.highestCombo = data_.highestCombo;
    
    // Send to lobby server to update player stats
    lobbyConnection_.sendGameResult(packet);
}
```

## Related Menus

- [LobbyMenu](lobby-menu.md) - Return destination
- [ProfileMenu](profile-menu.md) - View stats destination

## Related Components

- [TextComponent](../components/text-component.md)
- [ButtonComponent](../components/button-component.md)
- [BoxComponent](../components/box-component.md)

## Related Systems

- [GameOverSystem](../systems/game-over-system.md) - Triggers this menu
- [ButtonSystem](../systems/button-system.md)
- [RenderSystem](../systems/render-system.md)
