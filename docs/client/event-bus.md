# EventBus: Lightweight Publish/Subscribe

## Overview

The `EventBus` is a lightweight, type-safe publish/subscribe system that enables decoupled communication between systems. Instead of systems directly calling each other (tight coupling), they emit events that other systems can subscribe to (loose coupling).

## Purpose

The EventBus solves the problem of **system coupling** in game engines:

### ❌ Without EventBus (Tightly Coupled)

```cpp
class GameSystem {
    AudioSystem* audioSystem;
    HUDSystem* hudSystem;
    RenderSystem* renderSystem;

    void onEnemyKilled() {
        // Direct dependencies on all systems
        audioSystem->playSound("explosion.wav");
        hudSystem->addScore(100);
        renderSystem->spawnParticles(enemyPos);
    }
};
```

**Problems:**
- GameSystem must know about all other systems
- Hard to test in isolation
- Adding new reactions requires modifying GameSystem
- Circular dependencies become common

### ✅ With EventBus (Loosely Coupled)

```cpp
class GameSystem {
    EventBus* bus;

    void onEnemyKilled() {
        // Just emit an event
        bus->emit<EntityDestroyedEvent>(
            EntityDestroyedEvent{enemyId, "enemy"}
        );
    }
};

// Each system subscribes independently
audioSystem.subscribe<EntityDestroyedEvent>([](const auto& e) {
    playSound("explosion.wav");
});

hudSystem.subscribe<EntityDestroyedEvent>([](const auto& e) {
    addScore(100);
});

renderSystem.subscribe<EntityDestroyedEvent>([](const auto& e) {
    spawnParticles(e.x, e.y);
});
```

**Benefits:**
- Systems don't know about each other
- Easy to add new reactions without modifying existing code
- Simple to test each system in isolation
- No circular dependencies

## Architecture

```
┌──────────────────────────────────────────────────┐
│              EventBus                            │
│  ┌────────────────────────────────────────────┐ │
│  │  Channel<EntityDamagedEvent>              │ │
│  │  - subscribers: [fn1, fn2, fn3]           │ │
│  │  - current: []                             │ │
│  │  - next: [event1, event2]                  │ │
│  └────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────┐ │
│  │  Channel<PlaySoundEvent>                  │ │
│  │  - subscribers: [fn1]                      │ │
│  │  - current: []                             │ │
│  │  - next: [event1]                          │ │
│  └────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────┘

Emit Phase:          Process Phase:
emit() adds to      swapIn() moves next → current
"next" queue        run() calls all subscribers
                    Exceptions caught and ignored
```

## Event Categories

The R-Type client defines four categories of events:

### 1. Game Events ([GameEvents.hpp](../../client/include/events/GameEvents.hpp))

Core gameplay events:
- `EntitySpawnedEvent` - Entity created
- `EntityDestroyedEvent` - Entity removed
- `EntityDamagedEvent` - Entity took damage
- `PlayerScoredEvent` - Player gained points
- `PlayerDiedEvent` - Player lost a life
- `CollisionEvent` - Two entities collided
- `ProjectileFiredEvent` - Weapon fired
- `BossSpawnedEvent` / `BossDefeatedEvent` - Boss lifecycle
- `WaveStartedEvent` / `WaveCompletedEvent` - Wave system

### 2. Audio Events ([AudioEvents.hpp](../../client/include/events/AudioEvents.hpp))

Sound and music control:
- `PlaySoundEvent` - Play a sound effect
- `PlayMusicEvent` - Start background music
- `StopMusicEvent` - Stop current music
- `StopSoundEvent` - Stop specific sound
- `SetMasterVolumeEvent` - Adjust master volume
- `Play3DSoundEvent` - Positional audio

### 3. UI Events ([UIEvents.hpp](../../client/include/events/UIEvents.hpp))

HUD and interface updates:
- `ShowNotificationEvent` - Display toast message
- `UpdateScoreDisplayEvent` - Update score text
- `UpdateHealthDisplayEvent` - Update health bar
- `UpdateLivesDisplayEvent` - Update lives counter
- `ShowDamageIndicatorEvent` - Floating damage number
- `UpdateBossHealthBarEvent` - Boss health bar
- `ShowDialogEvent` - Display dialog box

### 4. Render Events ([RenderEvents.hpp](../../client/include/events/RenderEvents.hpp))

Visual effects and rendering:
- `CameraShakeEvent` - Shake the camera
- `ScreenFlashEvent` - Flash screen with color
- `SpawnParticleEffectEvent` - Create particle effect
- `PlayAnimationEvent` - Trigger animation
- `TintEntityEvent` - Apply color tint
- `HighlightEntityEvent` - Highlight entity
- `SetEntityVisibilityEvent` - Show/hide entity

## Usage

### Basic Pattern

```cpp
// 1. Create EventBus (typically in main game loop or application class)
EventBus bus;

// 2. Systems subscribe to events during initialization
class AudioSystem {
    void initialize(EventBus& bus) {
        bus.subscribe<PlaySoundEvent>([this](const PlaySoundEvent& e) {
            this->playSound(e.soundName, e.volume);
        });

        bus.subscribe<EntityDamagedEvent>([this](const EntityDamagedEvent& e) {
            this->playSound("hit.wav", 0.8F);
        });
    }
};

// 3. Emit events when things happen
void GameSystem::update() {
    if (enemyKilled) {
        bus.emit<EntityDestroyedEvent>(
            EntityDestroyedEvent{enemyId, "enemy"}
        );
    }
}

// 4. Process events once per frame (after all systems updated)
void GameLoop::run() {
    while (running) {
        // Update all systems (they emit events)
        gameSystem.update(deltaTime);
        physicsSystem.update(deltaTime);

        // Process all emitted events (subscribers react)
        bus.process();

        // Render
        renderSystem.render();
    }
}
```

### Example 1: Enemy Destruction

```cpp
// CollisionSystem detects enemy death
void CollisionSystem::update(Registry& registry, EventBus& bus) {
    for (auto [id, health] : registry.view<HealthComponent>()) {
        if (health.current <= 0) {
            auto& transform = registry.get<TransformComponent>(id);

            // Emit single event
            bus.emit<EntityDestroyedEvent>(
                EntityDestroyedEvent{id, "enemy"}
            );
        }
    }
}

// Multiple systems react independently:

// AudioSystem plays explosion sound
audioSystem.subscribe<EntityDestroyedEvent>([](const auto& e) {
    if (e.entityType == "enemy") {
        playSound("explosion.wav", 1.0F);
    }
});

// RenderSystem spawns particles
renderSystem.subscribe<EntityDestroyedEvent>([&](const auto& e) {
    auto& transform = registry.get<TransformComponent>(e.entityId);
    spawnParticles("explosion", transform.x, transform.y);
});

// HUDSystem updates score
hudSystem.subscribe<EntityDestroyedEvent>([&](const auto& e) {
    if (e.entityType == "enemy") {
        addScore(100);
    }
});

// GameSystem awards achievements
gameSystem.subscribe<EntityDestroyedEvent>([&](const auto& e) {
    totalKills++;
    if (totalKills == 100) {
        unlockAchievement("CENTURION");
    }
});
```

### Example 2: Player Damage with Multiple Effects

```cpp
void CombatSystem::applyDamage(EntityId target, int amount, EventBus& bus) {
    auto& health = registry.get<HealthComponent>(target);
    health.current -= amount;

    // Emit damage event
    bus.emit<EntityDamagedEvent>(
        EntityDamagedEvent{target, attackerId, amount, health.current}
    );
}

// Systems react:

// AudioSystem plays hurt sound
audioSystem.subscribe<EntityDamagedEvent>([](const auto& e) {
    playSound("hurt.wav", 0.7F);
});

// RenderSystem flashes entity red
renderSystem.subscribe<EntityDamagedEvent>([&](const auto& e) {
    tintEntity(e.entityId, 1.0F, 0.0F, 0.0F, 0.2F);
});

// RenderSystem shakes camera on heavy hits
renderSystem.subscribe<EntityDamagedEvent>([&](const auto& e) {
    if (e.damageAmount > 20) {
        bus.emit<CameraShakeEvent>(
            CameraShakeEvent{5.0F, 0.3F, 30.0F}
        );
    }
});

// HUDSystem shows damage number
hudSystem.subscribe<EntityDamagedEvent>([&](const auto& e) {
    auto& transform = registry.get<TransformComponent>(e.entityId);
    showFloatingDamage(transform.x, transform.y, e.damageAmount);
});

// HUDSystem updates health bar
hudSystem.subscribe<EntityDamagedEvent>([&](const auto& e) {
    updateHealthBar(e.entityId, e.remainingHealth);
});
```

### Example 3: Boss Fight

```cpp
void BossSystem::spawnBoss(EventBus& bus) {
    EntityId bossId = registry.createEntity();
    // ... create boss entity ...

    bus.emit<BossSpawnedEvent>(
        BossSpawnedEvent{bossId, "Mega Boss", 10000}
    );
}

// Multiple reactions:

// AudioSystem plays boss music
audioSystem.subscribe<BossSpawnedEvent>([&](const auto& e) {
    bus.emit<StopMusicEvent>(StopMusicEvent{2.0F}); // Fade out
    bus.emit<PlayMusicEvent>(
        PlayMusicEvent{"boss_theme.ogg", 1.0F, true, 2.0F}
    );
});

// HUDSystem shows boss health bar
hudSystem.subscribe<BossSpawnedEvent>([&](const auto& e) {
    bus.emit<UpdateBossHealthBarEvent>(
        UpdateBossHealthBarEvent{e.bossId, e.maxHealth, e.maxHealth, e.bossName}
    );
});

// RenderSystem plays dramatic camera effect
renderSystem.subscribe<BossSpawnedEvent>([&](const auto& e) {
    bus.emit<ScreenFlashEvent>(
        ScreenFlashEvent{1.0F, 0.0F, 0.0F, 0.5F, 0.5F}
    );
});

// UISystem shows warning
hudSystem.subscribe<BossSpawnedEvent>([&](const auto& e) {
    bus.emit<ShowNotificationEvent>(
        ShowNotificationEvent{
            "WARNING: Boss Approaching!",
            5.0F,
            ShowNotificationEvent::Type::Warning
        }
    );
});
```

## API Reference

### subscribe<EventType>(callback)

```cpp
template <typename T, typename F>
void subscribe(F&& callback);
```

Registers a callback to be invoked when `EventType` is emitted.

**Parameters:**
- `callback` - Function/lambda taking `const EventType&`

**Example:**
```cpp
bus.subscribe<PlaySoundEvent>([](const PlaySoundEvent& e) {
    std::cout << "Playing: " << e.soundName << std::endl;
});
```

**Note:** Subscribers are called in registration order.

---

### emit<EventType>(event)

```cpp
template <typename T>
void emit(const T& event);  // Copy

template <typename T>
void emit(T&& event);       // Move
```

Queues an event to be processed on next `process()` call.

**Parameters:**
- `event` - Event instance (copied or moved)

**Example:**
```cpp
// Copy
PlaySoundEvent evt{"laser.wav", 0.8F};
bus.emit<PlaySoundEvent>(evt);

// Move
bus.emit<PlaySoundEvent>(
    PlaySoundEvent{"laser.wav", 0.8F}
);
```

**Important:** Events are **not** delivered immediately. They're queued and delivered during `process()`.

---

### process()

```cpp
void process();
```

Processes all queued events by calling all registered subscribers.

**Behavior:**
1. Swaps "next" queue into "current" for each event type
2. Calls all subscribers with each event in order
3. Clears processed events
4. Events emitted during processing are deferred to next `process()` call

**Example:**
```cpp
void GameLoop::run() {
    while (running) {
        inputSystem.update(bus);
        gameSystem.update(bus);
        physicsSystem.update(bus);

        // Deliver all events emitted by systems
        bus.process();

        renderSystem.render();
    }
}
```

---

### clear()

```cpp
void clear();
```

Discards all pending events (current and next queues).

**Use case:** Switching game states (e.g., leaving level, going to menu).

---

## Advanced Patterns

### Pattern 1: Cascading Events

Events can trigger other events:

```cpp
// Enemy dies → spawn explosion
bus.subscribe<EntityDestroyedEvent>([&](const auto& e) {
    if (e.entityType == "enemy") {
        bus.emit<SpawnParticleEffectEvent>(
            SpawnParticleEffectEvent{"explosion", e.x, e.y}
        );
    }
});

// Explosion spawns → play sound
bus.subscribe<SpawnParticleEffectEvent>([&](const auto& e) {
    if (e.effectType == "explosion") {
        bus.emit<PlaySoundEvent>(
            PlaySoundEvent{"boom.wav", 0.9F}
        );
    }
});
```

**Important:** Cascaded events are processed in the **next** `process()` call, not immediately.

### Pattern 2: Event Filtering

Subscribers can ignore irrelevant events:

```cpp
bus.subscribe<EntityDamagedEvent>([&](const auto& e) {
    // Only react to player damage
    if (e.entityId != playerId) {
        return;
    }

    // Update player health UI
    updateHealthBar(e.remainingHealth);
});
```

### Pattern 3: Conditional Logic in Events

```cpp
struct EntityDamagedEvent {
    EntityId entityId;
    EntityId attacker;
    int damageAmount;
    int remainingHealth;

    bool isFatal() const { return remainingHealth <= 0; }
    bool isPlayer(EntityId playerId) const { return entityId == playerId; }
};

bus.subscribe<EntityDamagedEvent>([&](const auto& e) {
    if (e.isFatal()) {
        bus.emit<EntityDestroyedEvent>(/*...*/);
    }
});
```

### Pattern 4: System Initialization

```cpp
class GameSystem {
  public:
    void initialize(EventBus& bus) {
        // Subscribe to all events this system cares about
        bus.subscribe<EntitySpawnedEvent>([this](const auto& e) {
            this->onEntitySpawned(e);
        });

        bus.subscribe<EntityDestroyedEvent>([this](const auto& e) {
            this->onEntityDestroyed(e);
        });
    }

  private:
    void onEntitySpawned(const EntitySpawnedEvent& e) { /*...*/ }
    void onEntityDestroyed(const EntityDestroyedEvent& e) { /*...*/ }
};
```

## Best Practices

### ✅ DO

1. **Emit events from systems, not components**
   - Components are data, systems are logic
   - Systems have context to know when events should fire

2. **Use descriptive event names**
   - `PlayerScoredEvent` not `ScoreEvent`
   - `BossDefeatedEvent` not `BossEvent`

3. **Keep events immutable**
   - Events are POD structs, no methods that mutate state
   - Subscribers should not modify the event

4. **Process events once per frame**
   - Call `bus.process()` once at a fixed point in the game loop
   - Typically after all systems have updated

5. **Use event categories**
   - Separate files for GameEvents, AudioEvents, UIEvents, RenderEvents
   - Easier to navigate and understand

### ❌ DON'T

1. **Don't use events for synchronous communication**
   - Events are deferred until `process()`
   - For immediate results, use direct function calls

2. **Don't create circular event dependencies**
   - Event A triggers Event B triggers Event A → infinite loop
   - Use state flags to break cycles

3. **Don't mutate global state in subscribers without care**
   - Order of subscribers is deterministic but hard to track
   - Use events for notifications, not state synchronization

4. **Don't emit too many events**
   - Emitting 1000s of events per frame has overhead
   - Batch similar events or use direct calls for hot paths

5. **Don't forget to call process()**
   - Events only fire during `process()`
   - No-op without it

## Performance Considerations

### Memory

- Each event type has its own channel (type-erased storage)
- Events are stored in `std::vector` (contiguous, cache-friendly)
- Double-buffered queues avoid allocation during processing

### CPU

- `emit()` is cheap: single `push_back()` operation
- `subscribe()` is called once during initialization
- `process()` iterates subscribers × events

**Typical overhead:**
- Emit: ~5-10 CPU cycles (vector push)
- Process: ~20-50 cycles per subscriber call
- For 100 events × 5 subscribers = ~10,000 cycles (~3-5 µs on modern CPU)

### When to Avoid EventBus

Use direct function calls for:
- **Hot paths**: Physics integration, rendering loops
- **Real-time queries**: "Is player alive?" → check component directly
- **Large data**: Passing meshes, textures → use references/pointers
- **Synchronous results**: Need return value immediately

## Integration Example

```cpp
// Main.cpp
int main() {
    Registry registry;
    EventBus bus;

    // Create systems
    GameSystem gameSystem;
    AudioSystem audioSystem;
    HUDSystem hudSystem;
    RenderSystem renderSystem;

    // Initialize (subscribe to events)
    gameSystem.initialize(bus);
    audioSystem.initialize(bus);
    hudSystem.initialize(bus);
    renderSystem.initialize(bus);

    // Game loop
    Clock clock;
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        // Update systems (emit events)
        inputSystem.update(registry, bus, deltaTime);
        gameSystem.update(registry, bus, deltaTime);
        physicsSystem.update(registry, bus, deltaTime);

        // Process all emitted events
        bus.process();

        // Render
        renderSystem.render(registry, window);

        window.display();
    }

    return 0;
}
```

## Testing

The EventBus is fully tested in [EventBusIntegrationTests.cpp](../../tests/client/events/EventBusIntegrationTests.cpp):

- Basic emit/subscribe flow
- Multiple subscribers
- Event ordering
- Deferred event emission
- Clear functionality
- Complex gameplay scenarios

Run tests:
```bash
make test_client
./rtype_client_tests --gtest_filter="EventBusIntegration*"
```

## Debugging

### Enable Event Logging

```cpp
bus.subscribe<EntityDamagedEvent>([](const auto& e) {
    std::cout << "EntityDamaged: id=" << e.entityId
              << " damage=" << e.damageAmount << std::endl;
});
```

### Count Events

```cpp
int eventCount = 0;
bus.subscribe<EntityDamagedEvent>([&](const auto&) { eventCount++; });

// After process()
std::cout << "Processed " << eventCount << " damage events" << std::endl;
```

### Trace Event Chain

```cpp
bus.subscribe<EntityDestroyedEvent>([](const auto& e) {
    std::cout << "[1] Entity destroyed: " << e.entityId << std::endl;
});

bus.subscribe<PlaySoundEvent>([](const auto& e) {
    std::cout << "[2] Playing sound: " << e.soundName << std::endl;
});
```

## Common Issues

### Issue: Events not firing

**Check:**
1. Did you call `bus.process()`?
2. Are you emitting before subscribing?
3. Is the event type exactly matching? (e.g., `PlaySoundEvent` vs `PlayMusicEvent`)

### Issue: Events firing twice

**Check:**
1. Are you calling `bus.process()` multiple times per frame?
2. Did you subscribe the same callback twice?

### Issue: Subscriber not called

**Check:**
1. Is the subscriber registered for the correct event type?
2. Did you use the correct template parameter? `subscribe<PlaySoundEvent>` not `subscribe<PlayMusicEvent>`

## Complete System Examples

### Example: AudioSystem Implementation

```cpp
// AudioSystem.hpp
#pragma once

#include "events/AudioEvents.hpp"
#include "events/GameEvents.hpp"
#include "events/EventBus.hpp"
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>

class AudioSystem {
public:
    void initialize(EventBus& bus);
    void update(float deltaTime);

private:
    // Event handlers
    void onEntityDamaged(const EntityDamagedEvent& e);
    void onProjectileFired(const ProjectileFiredEvent& e);
    void onPlayerDied(const PlayerDiedEvent& e);
    void onBossSpawned(const BossSpawnedEvent& e);
    void onPlaySound(const PlaySoundEvent& e);
    void onPlayMusic(const PlayMusicEvent& e);
    void onStopMusic(const StopMusicEvent& e);

    // Audio resources
    std::unordered_map<std::string, sf::SoundBuffer> soundBuffers_;
    std::unordered_map<std::string, sf::Sound> sounds_;
    sf::Music music_;
};

// AudioSystem.cpp
void AudioSystem::initialize(EventBus& bus) {
    // Subscribe to game events
    bus.subscribe<EntityDamagedEvent>([this](const auto& e) {
        this->onEntityDamaged(e);
    });

    bus.subscribe<ProjectileFiredEvent>([this](const auto& e) {
        this->onProjectileFired(e);
    });

    bus.subscribe<PlayerDiedEvent>([this](const auto& e) {
        this->onPlayerDied(e);
    });

    bus.subscribe<BossSpawnedEvent>([this](const auto& e) {
        this->onBossSpawned(e);
    });

    // Subscribe to audio commands
    bus.subscribe<PlaySoundEvent>([this](const auto& e) {
        this->onPlaySound(e);
    });

    bus.subscribe<PlayMusicEvent>([this](const auto& e) {
        this->onPlayMusic(e);
    });

    bus.subscribe<StopMusicEvent>([this](const auto& e) {
        this->onStopMusic(e);
    });
}

void AudioSystem::onEntityDamaged(const EntityDamagedEvent& e) {
    // Play hit sound with volume based on damage
    float volume = std::min(1.0F, e.damageAmount / 50.0F);

    sf::Sound& sound = sounds_["hit"];
    sound.setVolume(volume * 100.0F);
    sound.play();
}

void AudioSystem::onProjectileFired(const ProjectileFiredEvent& e) {
    sounds_["laser"].play();
}

void AudioSystem::onPlayerDied(const PlayerDiedEvent& e) {
    sounds_["death"].play();
}

void AudioSystem::onBossSpawned(const BossSpawnedEvent& e) {
    // Stop current music and play boss theme
    music_.stop();

    if (music_.openFromFile("assets/music/boss_theme.ogg")) {
        music_.setLoop(true);
        music_.setVolume(80.0F);
        music_.play();
    }
}

void AudioSystem::onPlaySound(const PlaySoundEvent& e) {
    auto it = sounds_.find(e.soundName);
    if (it != sounds_.end()) {
        it->second.setVolume(e.volume * 100.0F);
        it->second.setPitch(e.pitch);
        it->second.setLoop(e.loop);
        it->second.play();
    }
}

void AudioSystem::onPlayMusic(const PlayMusicEvent& e) {
    music_.stop();

    if (music_.openFromFile("assets/music/" + e.musicName)) {
        music_.setLoop(e.loop);
        music_.setVolume(e.volume * 100.0F);
        music_.play();
    }
}

void AudioSystem::onStopMusic(const StopMusicEvent& e) {
    // TODO: Implement fade out
    music_.stop();
}
```

### Example: HUDSystem Implementation

```cpp
// HUDSystem.hpp
#pragma once

#include "events/UIEvents.hpp"
#include "events/GameEvents.hpp"
#include "events/EventBus.hpp"
#include "ecs/Registry.hpp"

class HUDSystem {
public:
    void initialize(EventBus& bus);
    void update(Registry& registry, float deltaTime);
    void render(sf::RenderWindow& window);

private:
    // Event handlers
    void onPlayerScored(const PlayerScoredEvent& e);
    void onUpdateHealthDisplay(const UpdateHealthDisplayEvent& e);
    void onUpdateScoreDisplay(const UpdateScoreDisplayEvent& e);
    void onShowNotification(const ShowNotificationEvent& e);
    void onUpdateBossHealthBar(const UpdateBossHealthBarEvent& e);
    void onHideBossHealthBar(const HideBossHealthBarEvent& e);

    // HUD state
    int currentScore_ = 0;
    int currentHealth_ = 100;
    int maxHealth_ = 100;

    // Boss bar
    bool showBossBar_ = false;
    std::string bossName_;
    int bossHealth_ = 0;
    int bossMaxHealth_ = 0;

    // Notifications
    struct Notification {
        std::string message;
        float remainingTime;
        ShowNotificationEvent::Type type;
    };
    std::vector<Notification> notifications_;
};

// HUDSystem.cpp
void HUDSystem::initialize(EventBus& bus) {
    bus.subscribe<PlayerScoredEvent>([this](const auto& e) {
        this->onPlayerScored(e);
    });

    bus.subscribe<UpdateHealthDisplayEvent>([this](const auto& e) {
        this->onUpdateHealthDisplay(e);
    });

    bus.subscribe<UpdateScoreDisplayEvent>([this](const auto& e) {
        this->onUpdateScoreDisplay(e);
    });

    bus.subscribe<ShowNotificationEvent>([this](const auto& e) {
        this->onShowNotification(e);
    });

    bus.subscribe<UpdateBossHealthBarEvent>([this](const auto& e) {
        this->onUpdateBossHealthBar(e);
    });

    bus.subscribe<HideBossHealthBarEvent>([this](const auto& e) {
        this->onHideBossHealthBar(e);
    });
}

void HUDSystem::onPlayerScored(const PlayerScoredEvent& e) {
    currentScore_ = e.totalScore;

    // Show score gain notification
    notifications_.push_back(Notification{
        "+" + std::to_string(e.pointsGained) + " points",
        2.0F,
        ShowNotificationEvent::Type::Success
    });
}

void HUDSystem::onUpdateHealthDisplay(const UpdateHealthDisplayEvent& e) {
    currentHealth_ = e.currentHealth;
    maxHealth_ = e.maxHealth;
}

void HUDSystem::onUpdateScoreDisplay(const UpdateScoreDisplayEvent& e) {
    currentScore_ = e.score;
}

void HUDSystem::onShowNotification(const ShowNotificationEvent& e) {
    notifications_.push_back(Notification{
        e.message,
        e.duration,
        e.type
    });
}

void HUDSystem::onUpdateBossHealthBar(const UpdateBossHealthBarEvent& e) {
    showBossBar_ = true;
    bossName_ = e.bossName;
    bossHealth_ = e.currentHealth;
    bossMaxHealth_ = e.maxHealth;
}

void HUDSystem::onHideBossHealthBar(const HideBossHealthBarEvent& e) {
    showBossBar_ = false;
}

void HUDSystem::update(Registry& registry, float deltaTime) {
    // Update notifications (fade out)
    for (auto it = notifications_.begin(); it != notifications_.end();) {
        it->remainingTime -= deltaTime;
        if (it->remainingTime <= 0.0F) {
            it = notifications_.erase(it);
        } else {
            ++it;
        }
    }
}
```

### Example: GameSystem with Event Emission

```cpp
// GameSystem.cpp
void GameSystem::update(Registry& registry, EventBus& bus, float deltaTime) {
    // Check for entity deaths
    for (auto [id, health, transform] : registry.view<HealthComponent, TransformComponent>()) {
        if (health.current <= 0 && health.wasAlive) {
            health.wasAlive = false;

            // Emit entity destroyed event
            bus.emit<EntityDestroyedEvent>(
                EntityDestroyedEvent{id, "enemy"}
            );

            // Award points
            bus.emit<PlayerScoredEvent>(
                PlayerScoredEvent{
                    playerId_,
                    100,              // points gained
                    playerScore_,     // total score
                    "enemy_kill"
                }
            );

            // Spawn particles
            bus.emit<SpawnParticleEffectEvent>(
                SpawnParticleEffectEvent{
                    "explosion",
                    transform.x,
                    transform.y
                }
            );

            // Play sound
            bus.emit<PlaySoundEvent>(
                PlaySoundEvent{"explosion.wav", 0.9F}
            );

            // Camera shake
            bus.emit<CameraShakeEvent>(
                CameraShakeEvent{5.0F, 0.3F, 30.0F}
            );
        }
    }

    // Check for wave completion
    if (enemiesRemaining_ == 0 && waveActive_) {
        waveActive_ = false;

        bus.emit<WaveCompletedEvent>(
            WaveCompletedEvent{
                currentWave_,
                enemiesKilled_,
                1000  // bonus points
            }
        );

        bus.emit<ShowNotificationEvent>(
            ShowNotificationEvent{
                "Wave " + std::to_string(currentWave_) + " Complete!",
                3.0F,
                ShowNotificationEvent::Type::Success
            }
        );
    }
}
```

### Example: Main Game Loop Integration

```cpp
// Main.cpp
int main() {
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "R-Type");

    // Create ECS registry and EventBus
    Registry registry;
    EventBus bus;

    // Create systems
    GameSystem gameSystem;
    AudioSystem audioSystem;
    HUDSystem hudSystem;
    RenderSystem renderSystem;
    PhysicsSystem physicsSystem;
    InputSystem inputSystem;

    // Initialize all systems (subscribe to events)
    gameSystem.initialize(bus);
    audioSystem.initialize(bus);
    hudSystem.initialize(bus);
    renderSystem.initialize(bus);

    // Game loop
    sf::Clock clock;
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        // Handle window events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // Update systems (they emit events during update)
        inputSystem.update(registry, bus, deltaTime);
        gameSystem.update(registry, bus, deltaTime);
        physicsSystem.update(registry, bus, deltaTime);
        hudSystem.update(registry, deltaTime);

        // Process all emitted events
        // This is when subscribers react to events
        bus.process();

        // Render
        window.clear();
        renderSystem.render(registry, window);
        hudSystem.render(window);
        window.display();
    }

    return 0;
}
```

## See Also

- [GameEvents.hpp](../../client/include/events/GameEvents.hpp) - Core gameplay events
- [AudioEvents.hpp](../../client/include/events/AudioEvents.hpp) - Sound/music events
- [UIEvents.hpp](../../client/include/events/UIEvents.hpp) - HUD/interface events
- [RenderEvents.hpp](../../client/include/events/RenderEvents.hpp) - Visual effects events
- [Event Bus (ECS Architecture)](../global-overview/ecs-architecture/event-bus.md) - Technical implementation details
