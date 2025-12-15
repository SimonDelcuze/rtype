# Asset Manifest & Loader

The asset manifest system allows you to define all game assets (textures, sounds, and fonts) in a JSON file and load them automatically at startup.

## Manifest Format

Create a JSON file (e.g., `assets.json`) describing your assets:

```json
{
  "textures": [
    {
      "id": "space_background",
      "path": "backgrounds/space.png",
      "type": "background"
    },
    {
      "id": "player_ship",
      "path": "sprites/player.png",
      "type": "sprite"
    }
  ],
  "sounds": [
    {
      "id": "laser",
      "path": "sounds/laser.wav",
      "type": "sfx"
    },
    {
      "id": "theme_music",
      "path": "music/theme.ogg",
      "type": "music"
    }
  ],
  "fonts": [
    {
      "id": "ui_font",
      "path": "fonts/ui.ttf",
      "type": "ui"
    },
    {
      "id": "score_font",
      "path": "fonts/score.ttf",
      "type": "game"
    }
  ]
}
```

### Fields

- **id**: Unique identifier for the asset (used in `TextureManager`, `SoundManager`, or `FontManager`)
- **path**: Relative path to the asset file
- **type**: Optional categorization ("sprite", "background", "sfx", "music", "ui", "game", etc.)

## Usage

### Basic Loading

```cpp
#include "assets/AssetLoader.hpp"
#include "audio/SoundManager.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"

TextureManager textureManager;
SoundManager soundManager;
FontManager fontManager;
AssetLoader loader(textureManager, soundManager, fontManager);

loader.loadFromManifestFile("client/assets/assets.json");

const sf::Texture* playerTexture = textureManager.get("player_ship");
const sf::SoundBuffer* laserSound = soundManager.get("laser");
const sf::Font* uiFont = fontManager.get("ui_font");
```

### Loading with Progress Callback

For displaying a loading screen:

```cpp
loader.loadFromManifestFile("client/assets/assets.json",
    [](std::size_t loaded, std::size_t total, const std::string& currentId) {
        float progress = (float)loaded / (float)total * 100.0f;
        std::cout << "Loading: " << currentId << " (" << progress << "%)\n";
    }
);
```

The callback reports progress across all asset types combined (textures, sounds, and fonts).

### Loading from AssetManifest Object

```cpp
#include "assets/AssetManifest.hpp"

AssetManifest manifest = AssetManifest::fromFile("assets.json");

auto sprites = manifest.getTexturesByType("sprite");
auto backgrounds = manifest.getTexturesByType("background");
auto sfx = manifest.getSoundsByType("sfx");
auto uiFonts = manifest.getFontsByType("ui");

for (const auto& entry : sprites) {
    textureManager.load(entry.id, entry.path);
}

for (const auto& entry : sfx) {
    soundManager.load(entry.id, entry.path);
}

for (const auto& entry : uiFonts) {
    fontManager.load(entry.id, entry.path);
}
```

## API Reference

### AssetManifest

Parses and provides access to the manifest data.

```cpp
class AssetManifest
{
public:
    static AssetManifest fromFile(const std::string& filepath);

    static AssetManifest fromString(const std::string& json);

    const std::vector<TextureEntry>& getTextures() const;

    std::vector<TextureEntry> getTexturesByType(const std::string& type) const;

    const std::vector<SoundEntry>& getSounds() const;

    std::vector<SoundEntry> getSoundsByType(const std::string& type) const;
};

struct TextureEntry
{
    std::string id;
    std::string path;
    std::string type;
};

struct SoundEntry
{
    std::string id;
    std::string path;
    std::string type;
};

struct FontEntry
{
    std::string id;
    std::string path;
    std::string type;
};
```

### AssetLoader

Loads textures, sounds, and fonts from a manifest into their respective managers.

```cpp
class AssetLoader
{
public:
    using ProgressCallback = std::function<void(std::size_t loaded, std::size_t total, const std::string& currentId)>;

    AssetLoader(TextureManager& textureManager, SoundManager& soundManager, FontManager& fontManager);

    // Load from manifest object
    void loadFromManifest(const AssetManifest& manifest);
    void loadFromManifest(const AssetManifest& manifest, ProgressCallback callback);

    // Load from manifest file
    void loadFromManifestFile(const std::string& filepath);
    void loadFromManifestFile(const std::string& filepath, ProgressCallback callback);
};
```

The loader processes textures, sounds, and fonts in sequence, reporting combined progress through the callback.

## Complete Example

```cpp
#include "assets/AssetLoader.hpp"
#include "audio/SoundManager.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"

int main() {
    TextureManager textureManager;
    SoundManager soundManager;
    FontManager fontManager;
    AssetLoader loader(textureManager, soundManager, fontManager);

    // Load all assets with progress
    std::cout << "Loading assets...\n";
    loader.loadFromManifestFile("client/assets/assets.json",
        [](std::size_t loaded, std::size_t total, const std::string& id) {
            std::cout << "[" << loaded << "/" << total << "] " << id << "\n";
        }
    );

    std::cout << "Loaded " << textureManager.size() << " textures, "
              << soundManager.size() << " sounds, and "
              << fontManager.size() << " fonts\n";

    // Use textures
    const sf::Texture* player = textureManager.get("player_ship");
    if (player) {
        // Create sprite, etc.
    }

    // Use sounds
    const sf::SoundBuffer* laser = soundManager.get("laser");
    if (laser) {
        sf::Sound laserSound(*laser);
        laserSound.play();
    }

    // Use fonts
    const sf::Font* uiFont = fontManager.get("ui_font");
    if (uiFont) {
        sf::Text scoreText;
        scoreText.setFont(*uiFont);
        scoreText.setString("Score: 0");
    }

    return 0;
}
```

## Error Handling

Both `AssetManifest::fromFile()` and `AssetLoader::loadFromManifest()` throw `std::runtime_error` on errors:

- Invalid JSON syntax
- Missing required fields (id, path)
- File not found
- Texture loading failure

```cpp
try {
    loader.loadFromManifestFile("assets.json");
} catch (const std::runtime_error& e) {
    std::cerr << "Failed to load assets: " << e.what() << "\n";
}
```

## Benefits

1. **Centralized Configuration**: All assets (textures, sounds, and fonts) defined in one place
2. **Easy Maintenance**: Add/remove assets without changing code
3. **Loading Progress**: Track loading with callbacks across all asset types
4. **Type Filtering**: Load only specific asset types (e.g., only sprites, only sfx, only ui fonts)
5. **Error Handling**: Clear error messages for missing/invalid assets
6. **Unified Loading**: Single API for textures, sounds, and fonts
