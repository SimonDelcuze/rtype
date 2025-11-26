# Client Error Handling

The R-Type client uses custom exception classes that inherit from `IError` (defined in `shared/include/errors/IError.hpp`) to provide clear, specific error messages and improve debugging.

## Exception Hierarchy

All client exceptions inherit from `IError`, which itself inherits from `std::exception`:

```
std::exception
    └── IError
        ├── AssetLoadError
        ├── ManifestParseError
        ├── FileNotFoundError
        ├── RenderError
        └── AudioError
```

## Exception Types

### AssetLoadError

**Location**: [client/include/errors/AssetLoadError.hpp](../../client/include/errors/AssetLoadError.hpp)

Thrown when an asset (texture, sound, or font) fails to load from disk.

**Used by**:
- `TextureManager::load()` - When texture file cannot be loaded
- `SoundManager::load()` - When sound file cannot be loaded
- `FontManager::load()` - When font file cannot be loaded

**Example**:
```cpp
#include "errors/AssetLoadError.hpp"
#include "graphics/TextureManager.hpp"

try {
    TextureManager manager;
    manager.load("player", "sprites/player.png");
} catch (const AssetLoadError& e) {
    std::cerr << "Failed to load asset: " << e.message() << "\n";
    // Handle error - use fallback texture, log error, etc.
}
```

### ManifestParseError

**Location**: [client/include/errors/ManifestParseError.hpp](../../client/include/errors/ManifestParseError.hpp)

Thrown when an asset manifest file contains invalid JSON or missing required fields.

**Used by**:
- `AssetManifest::fromString()` - When JSON parsing fails
- `AssetManifest::fromString()` - When required fields (id, path) are missing
- `AssetManifest::fromFile()` - When file contains invalid JSON

**Example**:
```cpp
#include "assets/AssetManifest.hpp"
#include "errors/ManifestParseError.hpp"

try {
    std::string json = R"({"textures": [{"id": "", "path": ""}]})";
    AssetManifest manifest = AssetManifest::fromString(json);
} catch (const ManifestParseError& e) {
    std::cerr << "Invalid manifest: " << e.message() << "\n";
    // Handle error - use default assets, show error dialog, etc.
}
```

### FileNotFoundError

**Location**: [client/include/errors/FileNotFoundError.hpp](../../client/include/errors/FileNotFoundError.hpp)

Thrown when a file cannot be found or opened.

**Used by**:
- `AssetManifest::fromFile()` - When manifest file doesn't exist or can't be opened
- `AssetLoader::loadFromManifestFile()` - When manifest file is not found

**Example**:
```cpp
#include "assets/AssetManifest.hpp"
#include "errors/FileNotFoundError.hpp"

try {
    AssetManifest manifest = AssetManifest::fromFile("assets/config.json");
} catch (const FileNotFoundError& e) {
    std::cerr << "File not found: " << e.message() << "\n";
    // Handle error - create default config, show error, etc.
}
```

### RenderError

**Location**: [client/include/errors/RenderError.hpp](../../client/include/errors/RenderError.hpp)

Thrown when rendering operations fail (window creation, rendering, etc.).

**Reserved for future use** in rendering systems.

**Example**:
```cpp
#include "errors/RenderError.hpp"

try {
    // Future rendering operations
    createWindow(1920, 1080);
} catch (const RenderError& e) {
    std::cerr << "Rendering failed: " << e.message() << "\n";
}
```

### AudioError

**Location**: [client/include/errors/AudioError.hpp](../../client/include/errors/AudioError.hpp)

Thrown when audio operations fail (playback, audio device issues, etc.).

**Reserved for future use** in audio systems.

**Example**:
```cpp
#include "errors/AudioError.hpp"

try {
    // Future audio operations
    initializeAudioDevice();
} catch (const AudioError& e) {
    std::cerr << "Audio error: " << e.message() << "\n";
}
```

## Best Practices

### 1. Catch Specific Exceptions

Catch the most specific exception type first, then fall back to more general types:

```cpp
try {
    AssetLoader loader(textureManager, soundManager, fontManager);
    loader.loadFromManifestFile("assets.json");
} catch (const FileNotFoundError& e) {
    // Specific handling for missing file
    std::cerr << "Manifest file not found: " << e.message() << "\n";
    useDefaultAssets();
} catch (const ManifestParseError& e) {
    // Specific handling for invalid JSON
    std::cerr << "Invalid manifest format: " << e.message() << "\n";
    showErrorDialog(e.message());
} catch (const AssetLoadError& e) {
    // Specific handling for asset loading failures
    std::cerr << "Failed to load asset: " << e.message() << "\n";
    useFallbackAssets();
} catch (const IError& e) {
    // Catch-all for any IError-derived exception
    std::cerr << "Error: " << e.message() << "\n";
}
```

### 2. Use what() for C-Style Interfaces

The `what()` method (from `std::exception`) returns a `const char*`, useful for C-style APIs:

```cpp
try {
    manager.load("asset", "path.png");
} catch (const AssetLoadError& e) {
    // Use message() for std::string
    std::string msg = e.message();

    // Use what() for const char*
    logError(e.what());
}
```

### 3. Provide Context in Error Messages

When throwing custom exceptions, include relevant context:

```cpp
if (!texture.loadFromFile(path)) {
    throw AssetLoadError("Failed to load texture at path: " + path);
}
```

### 4. Don't Catch and Ignore

Always handle exceptions appropriately - log them, use fallbacks, or propagate them:

```cpp
// ❌ Bad - silently ignores error
try {
    manager.load("texture", "missing.png");
} catch (...) {
    // Do nothing
}

// ✅ Good - handles error with fallback
try {
    manager.load("texture", "missing.png");
} catch (const AssetLoadError& e) {
    std::cerr << "Warning: " << e.message() << "\n";
    manager.load("texture", "fallback.png");
}
```

### 5. RAII for Exception Safety

Use RAII to ensure resources are properly released even when exceptions occur:

```cpp
class ResourceGuard {
public:
    ResourceGuard(TextureManager& mgr, const std::string& id)
        : manager_(mgr), id_(id) {}

    ~ResourceGuard() {
        if (!committed_) {
            manager_.remove(id_);  // Clean up on exception
        }
    }

    void commit() { committed_ = true; }

private:
    TextureManager& manager_;
    std::string id_;
    bool committed_ = false;
};
```

## Error Handling Patterns

### Pattern 1: Graceful Degradation

Load fallback assets when primary assets fail:

```cpp
const sf::Texture* loadTextureWithFallback(
    TextureManager& manager,
    const std::string& id,
    const std::string& primaryPath,
    const std::string& fallbackPath)
{
    try {
        return &manager.load(id, primaryPath);
    } catch (const AssetLoadError&) {
        try {
            return &manager.load(id, fallbackPath);
        } catch (const AssetLoadError& e) {
            throw AssetLoadError("Failed to load both primary and fallback texture: " + e.message());
        }
    }
}
```

### Pattern 2: Error Aggregation

Collect multiple errors during batch operations:

```cpp
struct LoadResult {
    std::vector<std::string> loaded;
    std::vector<std::pair<std::string, std::string>> errors;  // id, error message
};

LoadResult loadAllTextures(TextureManager& manager, const std::vector<TextureEntry>& entries) {
    LoadResult result;

    for (const auto& entry : entries) {
        try {
            manager.load(entry.id, entry.path);
            result.loaded.push_back(entry.id);
        } catch (const AssetLoadError& e) {
            result.errors.push_back({entry.id, e.message()});
        }
    }

    return result;
}
```

### Pattern 3: Validation Before Loading

Check preconditions and throw early:

```cpp
void validateAssetPath(const std::string& path) {
    if (path.empty()) {
        throw AssetLoadError("Asset path cannot be empty");
    }
    if (!std::filesystem::exists(path)) {
        throw FileNotFoundError("Asset file does not exist: " + path);
    }
}
```

## Testing Exception Behavior

Use GTest's `EXPECT_THROW` to verify exceptions are thrown correctly:

```cpp
#include "errors/AssetLoadError.hpp"
#include <gtest/gtest.h>

TEST(TextureManager, LoadThrowsOnMissingFile)
{
    TextureManager manager;
    EXPECT_THROW(
        manager.load("missing", "nonexistent.png"),
        AssetLoadError
    );
}

TEST(AssetManifest, InvalidJSONThrows)
{
    std::string invalidJson = "{invalid}";
    EXPECT_THROW(
        AssetManifest::fromString(invalidJson),
        ManifestParseError
    );
}
```

## Migration from std::runtime_error

If you encounter old code using `std::runtime_error`, update it to use the appropriate custom exception:

```cpp
// ❌ Old code
if (!texture.loadFromFile(path)) {
    throw std::runtime_error("Failed to load texture");
}

// ✅ Updated code
if (!texture.loadFromFile(path)) {
    throw AssetLoadError("Failed to load texture at path: " + path);
}
```

Update catch blocks accordingly:

```cpp
// ❌ Old code
try {
    manager.load("id", "path");
} catch (const std::runtime_error& e) {
    // ...
}

// ✅ Updated code
try {
    manager.load("id", "path");
} catch (const AssetLoadError& e) {
    // ...
}
```

## See Also

- [Asset Manifest & Loader](../asset-manifest.md)
- [IError Interface](../../shared/include/errors/IError.hpp)
- [Component Errors](../../shared/include/errors/ComponentNotFoundError.hpp)
- [Registry Errors](../../shared/include/errors/RegistryError.hpp)
