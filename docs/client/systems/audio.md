# AudioSystem

Processes AudioComponent actions and plays sounds using SFML audio.

## Overview

The AudioSystem reads AudioComponent actions each frame and executes them:
- **Play**: Creates a sound from the loaded buffer and starts playback
- **Stop**: Stops the sound
- **Pause**: Pauses the sound

## Class Definition

```cpp
#include "systems/AudioSystem.hpp"

class AudioSystem
{
public:
    explicit AudioSystem(SoundManager& soundManager);
    void update(Registry& registry);
};
```

## Dependencies

### SoundManager

Manages loading and caching of sound buffers.

```cpp
#include "audio/SoundManager.hpp"

class SoundManager
{
public:
    // Load a sound buffer from file
    // Throws std::runtime_error if file cannot be loaded
    const sf::SoundBuffer& load(const std::string& id, const std::string& filepath);

    // Get a loaded sound buffer (returns nullptr if not found)
    const sf::SoundBuffer* get(const std::string& id) const;

    // Check if a sound is loaded
    bool has(const std::string& id) const;

    // Remove a loaded sound
    void remove(const std::string& id);

    // Clear all loaded sounds
    void clear();
};
```

## Usage

```cpp
// Setup
SoundManager soundManager;
soundManager.load("laser", "assets/sounds/laser.wav");
soundManager.load("explosion", "assets/sounds/explosion.wav");

AudioSystem audioSystem(soundManager);

// In game loop
audioSystem.update(registry);
```

## Behavior

When processing an AudioComponent:

| Action | Behavior |
|--------|----------|
| `Play` | Creates sound from buffer, applies settings (volume, pitch, loop), starts playback, sets `isPlaying = true` |
| `Stop` | Stops the sound, sets `isPlaying = false` |
| `Pause` | Pauses the sound, sets `isPlaying = false` |
| `None` | No action taken |

After processing, the action is reset to `None`.

The system also monitors playing sounds and updates `isPlaying` to `false` when a non-looping sound finishes.

## Complete Example

```cpp
#include "audio/SoundManager.hpp"
#include "components/AudioComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/AudioSystem.hpp"

// Setup
SoundManager soundManager;
soundManager.load("laser", "assets/sounds/laser.wav");
soundManager.load("music", "assets/sounds/background.ogg");

AudioSystem audioSystem(soundManager);
Registry registry;

// Create background music
EntityId musicEntity = registry.createEntity();
auto& musicAudio = registry.emplace<AudioComponent>(musicEntity);
musicAudio.volume = 30.0F;
musicAudio.loop = true;
musicAudio.play("music");

// Create player
EntityId player = registry.createEntity();
auto& playerAudio = registry.emplace<AudioComponent>(player);

// Game loop
while (running) {
    if (playerShooting) {
        playerAudio.play("laser");
    }

    audioSystem.update(registry);
}
```
