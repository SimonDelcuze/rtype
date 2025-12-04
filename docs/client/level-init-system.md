# Level Init System

The Level Init System handles `SERVER_LEVEL_INIT (0x30)` messages from the server to configure level-specific assets and entity types before gameplay begins.

## Overview

When the server sends a `LevelInit` message, the client:
1. Parses the binary payload to extract level metadata
2. Resolves asset IDs (sprites, backgrounds, music) via the local manifest
3. Builds an `EntityTypeRegistry` mapping `typeId` to render data (textures + animation)
4. Updates the current `LevelState`

## Components

### LevelInitData

```cpp
struct ArchetypeEntry
{
    std::uint16_t typeId;
    std::string spriteId;
    std::string animId;
    std::uint8_t layer;
};

struct LevelInitData
{
    std::uint16_t levelId;
    std::uint32_t seed;
    std::string backgroundId;
    std::string musicId;
    std::vector<ArchetypeEntry> archetypes;
};
```

### LevelInitParser

Static parser that decodes the binary `LevelInit` payload following the protocol specification:

```
u16 levelId
u32 seed
u8  backgroundIdLen, backgroundId[Len]
u8  musicIdLen, musicId[Len]
u8  archetypeCount
repeat archetypeCount times:
    u16 typeId
    u8  spriteIdLen, spriteId[Len]
    u8  animIdLen, animId[Len]
    u8  layer
```

### EntityTypeRegistry

Maps `typeId` (from snapshots) to render data:

```cpp
struct RenderTypeData
{
    const sf::Texture* texture;
    std::uint8_t frameCount;
    float frameDuration;
    std::uint8_t layer;
    std::uint32_t frameWidth;
    std::uint32_t frameHeight;
    std::uint32_t columns;
    const AnimationClip* animation; // optional, from animation registry
    std::string spriteId;
};

class EntityTypeRegistry
{
    void registerType(std::uint16_t typeId, RenderTypeData data);
    const RenderTypeData* get(std::uint16_t typeId) const;
    void clear();
};
```

### LevelState

Simple struct tracking current level:

```cpp
struct LevelState
{
    std::uint16_t levelId;
    std::uint32_t seed;
    bool active;
};
```

## Asset Resolution

When processing a `LevelInit`:

1. For each archetype's `spriteId`:
   - Look up in `AssetManifest` via `findTextureById()`
   - If found, load texture via `TextureManager`
   - If not found, log warning and use placeholder (magenta 32x32)

2. Resolve animation clip:
   - If `animId` is provided, first try exact clip id in the animation registry.
   - If not found, try `labels[spriteId][animId]` to map a label (e.g. `row4`) to a clip id.
   - Fallback: use the clip whose id matches `spriteId`, or no animation if none exists.

3. Populate `RenderTypeData` with:
   - Texture pointer
   - `frameCount`, `frameDuration`, `frameWidth/Height`, `columns` from the chosen clip if any
   - `animation` pointer to the clip (for custom frames)
   - Layer from the archetype

4. The placeholder ensures the game continues running even with missing assets

## Integration

```cpp
// In Main.cpp
EntityTypeRegistry typeRegistry;
LevelState levelState{};

gameLoop.addSystem(std::make_shared<LevelInitSystem>(
    net.levelInit, typeRegistry, manifest, textureManager, levelState));
```

## Data Flow

```
NetworkReceiver → NetworkMessageHandler → LevelInitParser
                                              ↓
                                    LevelInitData Queue
                                              ↓
                              LevelInitSystem.update()
                                              ↓
                  ┌───────────────────────────┼───────────────────────────┐
                  ↓                           ↓                           ↓
           LevelState update         EntityTypeRegistry         Asset preloading
           (levelId, seed)           (typeId → texture)         via TextureManager
                                   + animation clip
```

## Related

- [Network Protocol - LevelInit](../protocol/level-messages.md)
- [Snapshot Parser](snapshot-parser.md)
- [Asset Manifest](../asset-manifest.md)
