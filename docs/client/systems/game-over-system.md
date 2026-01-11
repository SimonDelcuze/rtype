# GameOverSystem

## Overview

The `GameOverSystem` is a game flow management system in the R-Type client that monitors game state conditions and triggers game-over events when appropriate conditions are met (such as all players dying or the game timer expiring). It coordinates with the event bus to notify other systems and UI components when the game has ended.

## Purpose and Design Philosophy

Game-over detection requires centralized logic:

1. **State Monitoring**: Watches for game-ending conditions
2. **Event Publishing**: Notifies other systems via event bus
3. **Single Trigger**: Ensures game-over only fires once
4. **Transition Coordination**: Initiates smooth transition to game-over menu
5. **Score Preservation**: Maintains final score for display

## Class Definition

```cpp
class GameOverSystem : public ISystem
{
  public:
    explicit GameOverSystem(EventBus& eventBus);

    void initialize() override {}
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override {}

  private:
    EventBus& eventBus_;
    bool gameOverTriggered_ = false;
};
```

## Constructor

```cpp
explicit GameOverSystem(EventBus& eventBus);
```

**Parameters:**
- `eventBus`: Reference to the global event bus for publishing game-over events

**Example:**
```cpp
GameOverSystem gameOverSystem(eventBus);
scheduler.addSystem(&gameOverSystem);
```

## Main Update Logic

```cpp
void GameOverSystem::update(Registry& registry, float deltaTime)
{
    // Only trigger once
    if (gameOverTriggered_) return;
    
    // Check for game-over conditions
    if (checkAllPlayersDead(registry) || checkGameTimerExpired(registry)) {
        triggerGameOver(registry);
    }
}
```

## Game-Over Conditions

### All Players Dead

```cpp
bool GameOverSystem::checkAllPlayersDead(Registry& registry)
{
    auto view = registry.view<PlayerComponent, LivesComponent>();
    
    for (auto entity : view) {
        auto& lives = view.get<LivesComponent>(entity);
        
        // At least one player has lives remaining
        if (lives.current > 0) {
            return false;
        }
    }
    
    // No players with lives found - game over
    return true;
}
```

### Game Timer Expired

```cpp
bool GameOverSystem::checkGameTimerExpired(Registry& registry)
{
    auto view = registry.view<GameTimerComponent>();
    
    for (auto entity : view) {
        auto& timer = view.get<GameTimerComponent>(entity);
        
        if (timer.enabled && timer.remainingTime <= 0) {
            return true;
        }
    }
    
    return false;
}
```

### Level Complete (Victory)

```cpp
bool GameOverSystem::checkLevelComplete(Registry& registry)
{
    auto view = registry.view<LevelStateComponent>();
    
    for (auto entity : view) {
        auto& levelState = view.get<LevelStateComponent>(entity);
        
        if (levelState.status == LevelStatus::Complete) {
            return true;
        }
    }
    
    return false;
}
```

## Triggering Game Over

```cpp
void GameOverSystem::triggerGameOver(Registry& registry)
{
    gameOverTriggered_ = true;
    
    // Gather final statistics
    GameOverData data;
    data.victory = checkLevelComplete(registry);
    data.finalScore = calculateFinalScore(registry);
    data.playTime = calculatePlayTime(registry);
    data.enemiesDefeated = countEnemiesDefeated(registry);
    
    // Publish game-over event
    eventBus_.publish(GameOverEvent{data});
    
    // Optionally pause game logic
    eventBus_.publish(PauseGameEvent{true});
}
```

## Game Over Event Data

```cpp
struct GameOverData
{
    bool victory;
    uint32_t finalScore;
    float playTime;
    uint32_t enemiesDefeated;
    uint32_t bossesDefeated;
    uint32_t highestCombo;
    std::string playerName;
};

struct GameOverEvent
{
    GameOverData data;
};
```

## Score Calculation

```cpp
uint32_t GameOverSystem::calculateFinalScore(Registry& registry)
{
    uint32_t totalScore = 0;
    
    auto view = registry.view<ScoreComponent>();
    for (auto entity : view) {
        totalScore += view.get<ScoreComponent>(entity).value;
    }
    
    return totalScore;
}
```

## Event Bus Integration

Other systems can subscribe to the game-over event:

```cpp
// In MenuSystem
void MenuSystem::initialize()
{
    eventBus_.subscribe<GameOverEvent>([this](const GameOverEvent& event) {
        onGameOver(event.data);
    });
}

void MenuSystem::onGameOver(const GameOverData& data)
{
    // Transition to game-over menu
    m_currentMenu = MenuState::GameOver;
    
    // Store data for display
    m_gameOverData = data;
    
    // Create game-over UI elements
    createGameOverUI(data);
}
```

### Audio System Response

```cpp
void AudioSystem::onGameOver(const GameOverEvent& event)
{
    // Stop game music
    m_bgMusic.stop();
    
    // Play appropriate jingle
    if (event.data.victory) {
        playSound("victory_jingle");
    } else {
        playSound("game_over_jingle");
    }
}
```

### Network System Response

```cpp
void NetworkSystem::onGameOver(const GameOverEvent& event)
{
    // Send final score to server
    GameOverPacket packet;
    packet.playerId = m_localPlayerId;
    packet.finalScore = event.data.finalScore;
    packet.victory = event.data.victory;
    
    m_socket.send(packet);
}
```

## Reset for New Game

```cpp
void GameOverSystem::reset()
{
    gameOverTriggered_ = false;
}

// Called when starting a new game
void GameManager::startNewGame()
{
    gameOverSystem_.reset();
    // ... reset other systems and entities
}
```

## Multiplayer Considerations

For multiplayer, game-over conditions may differ:

```cpp
bool GameOverSystem::checkMultiplayerGameOver(Registry& registry)
{
    // Count alive players
    int alivePlayers = 0;
    auto view = registry.view<PlayerComponent, LivesComponent>();
    
    for (auto entity : view) {
        auto& lives = view.get<LivesComponent>(entity);
        if (lives.current > 0) {
            alivePlayers++;
        }
    }
    
    // Different rules based on game mode
    switch (m_gameMode) {
        case GameMode::Cooperative:
            // All players must be dead
            return alivePlayers == 0;
            
        case GameMode::LastManStanding:
            // One or zero players remaining
            return alivePlayers <= 1;
            
        case GameMode::SharedLives:
            // Check shared lives pool
            return getSharedLives(registry) == 0;
    }
    
    return false;
}
```

## Delayed Game Over

For dramatic effect, delay the game-over screen:

```cpp
void GameOverSystem::update(Registry& registry, float deltaTime)
{
    if (gameOverTriggered_) {
        // Already triggered, wait for delay
        m_gameOverDelay -= deltaTime;
        if (m_gameOverDelay <= 0) {
            publishGameOverEvent(registry);
        }
        return;
    }
    
    if (checkGameOverCondition(registry)) {
        gameOverTriggered_ = true;
        m_gameOverDelay = 2.0F;  // 2 second delay for death animation
        
        // Optionally trigger slow-motion
        eventBus_.publish(SlowMotionEvent{0.5F, 2.0F});
    }
}
```

## Best Practices

1. **Single Trigger**: Use flag to prevent multiple game-over events
2. **Data Collection**: Gather all stats before publishing event
3. **Delayed Transition**: Allow death animations to complete
4. **Audio Sync**: Coordinate with audio for appropriate music
5. **Network Sync**: Send results to server in multiplayer

## Complete Flow

```
1. Game Running
        │
        ▼
2. Last player dies
        │
        ▼
3. GameOverSystem detects condition
        │
        ▼
4. Set gameOverTriggered_ = true
        │
        ▼
5. Collect final statistics
        │
        ▼
6. Publish GameOverEvent to EventBus
        │
        ├──────────────────────────────────────┐
        ▼                                       ▼
7. MenuSystem receives event           8. AudioSystem receives event
   - Create game-over UI                  - Stop game music
   - Display score                        - Play game-over jingle
        │                                       │
        └──────────────┬───────────────────────┘
                       ▼
9. Game Over screen displayed
```

## Related Components

- [LivesComponent](../components/lives-component.md) - Player lives tracking
- [ScoreComponent](../components/score-component.md) - Score data
- [PlayerComponent](../server/components/player-component.md) - Player identification

## Related Systems

- [HUDSystem](hud-system.md) - Displays game-over UI
- [AudioSystem](audio-system.md) - Plays game-over sounds
- [MenuRunner](../ui/menu-runner.md) - Menu state transitions
