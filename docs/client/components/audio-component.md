# AudioComponent

Controls sound playback for an entity.

## Structure

```cpp
#include "components/AudioComponent.hpp"

enum class AudioAction : std::uint8_t
{
    None,   // No action
    Play,   // Start playing
    Stop,   // Stop playing
    Pause   // Pause playback
};

struct AudioComponent
{
    std::string soundId;           // ID of the sound buffer to play
    AudioAction action;            // Current action
    float volume   = 100.0F;       // Volume (0-100)
    float pitch    = 1.0F;         // Pitch multiplier
    bool loop      = false;        // Loop the sound
    bool isPlaying = false;        // Read-only: whether sound is currently playing
};
```

## Methods

```cpp
// Create with sound ID preset
static AudioComponent AudioComponent::create(const std::string& soundId);

// Control methods
void play();                              // Start playing (uses current soundId)
void play(const std::string& id);         // Set soundId and start playing
void stop();                              // Stop playback
void pause();                             // Pause playback
```

## Usage Example

```cpp
// Create entity with audio
EntityId entity = registry.createEntity();
auto& audio = registry.emplace<AudioComponent>(entity);

// Play a sound
audio.play("explosion");

// Adjust settings before playing
audio.volume = 50.0F;
audio.pitch = 1.2F;
audio.loop = true;
audio.play("music");

// Stop sound
audio.stop();
```

## How It Works

The AudioComponent acts as a command buffer:

1. You set `soundId`, `volume`, `pitch`, `loop` as desired
2. You call `play()`, `stop()`, or `pause()` to set the action
3. The `AudioSystem` reads the action during its update and executes it
4. After processing, the action is reset to `None`

The `isPlaying` field is updated by the AudioSystem and reflects whether the sound is currently playing.
