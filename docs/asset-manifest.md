# Asset Manifest & Loader

The asset manifest system allows you to define all game assets in a JSON file and load them automatically at startup.

## Manifest Format

Create a JSON file (e.g., `assets.json`) describing your textures:

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
    },
    {
      "id": "enemy_ship",
      "path": "sprites/enemy.png",
      "type": "sprite"
    }
  ]
}
```

### Fields

- **id**: Unique identifier for the texture (used in `TextureManager`)
- **path**: Relative path to the texture file
- **type**: Optional categorization ("sprite", "background", etc.)

## Usage

### Basic Loading

```cpp
#include "assets/AssetLoader.hpp"
#include "graphics/TextureManager.hpp"

TextureManager textureManager;
AssetLoader loader(textureManager);

// Load all assets from manifest
loader.loadFromManifestFile("client/assets/assets.json");

// Access loaded textures
const sf::Texture* playerTexture = textureManager.get("player_ship");
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

### Loading from AssetManifest Object

```cpp
#include "assets/AssetManifest.hpp"

// Parse manifest
AssetManifest manifest = AssetManifest::fromFile("assets.json");

// Filter by type
auto sprites = manifest.getTexturesByType("sprite");
auto backgrounds = manifest.getTexturesByType("background");

// Load only specific types
for (const auto& entry : sprites) {
    textureManager.load(entry.id, entry.path);
}
```

## API Reference

### AssetManifest

Parses and provides access to the manifest data.

```cpp
class AssetManifest
{
public:
    // Load manifest from file
    static AssetManifest fromFile(const std::string& filepath);

    // Parse manifest from JSON string
    static AssetManifest fromString(const std::string& json);

    // Get all texture entries
    const std::vector<TextureEntry>& getTextures() const;

    // Get textures filtered by type
    std::vector<TextureEntry> getTexturesByType(const std::string& type) const;
};

struct TextureEntry
{
    std::string id;
    std::string path;
    std::string type;
};
```

### AssetLoader

Loads textures from a manifest into a TextureManager.

```cpp
class AssetLoader
{
public:
    using ProgressCallback = std::function<void(std::size_t loaded, std::size_t total, const std::string& currentId)>;

    explicit AssetLoader(TextureManager& textureManager);

    // Load from manifest object
    void loadFromManifest(const AssetManifest& manifest);
    void loadFromManifest(const AssetManifest& manifest, ProgressCallback callback);

    // Load from manifest file
    void loadFromManifestFile(const std::string& filepath);
    void loadFromManifestFile(const std::string& filepath, ProgressCallback callback);
};
```

## Complete Example

```cpp
#include "assets/AssetLoader.hpp"
#include "graphics/TextureManager.hpp"

int main() {
    TextureManager textureManager;
    AssetLoader loader(textureManager);

    // Load all assets with progress
    std::cout << "Loading assets...\n";
    loader.loadFromManifestFile("client/assets/assets.json",
        [](std::size_t loaded, std::size_t total, const std::string& id) {
            std::cout << "[" << loaded << "/" << total << "] " << id << "\n";
        }
    );

    std::cout << "Loaded " << textureManager.size() << " textures\n";

    // Use textures
    const sf::Texture* player = textureManager.get("player_ship");
    if (player) {
        // Create sprite, etc.
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

1. **Centralized Configuration**: All assets defined in one place
2. **Easy Maintenance**: Add/remove assets without changing code
3. **Loading Progress**: Track loading with callbacks
4. **Type Filtering**: Load only specific asset types
5. **Error Handling**: Clear error messages for missing/invalid assets
