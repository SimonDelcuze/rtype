# Levels, Waves & Spawn Scripts

## Overview

Enemy spawns are fully scripted on the server. The flow:
- `MonsterSpawnSystem` consumes a vector of `MovementComponent` patterns and a vector of `SpawnEvent`.
- `ObstacleSpawnSystem` consumes a vector of `ObstacleSpawn` (time, x, anchor [top/bottom/absolute], margin, health, speedX, hitbox, typeId) to drop blockers that scroll with the playfield (default `speedX = -50`) and are indestructible (only players take damage on contact).
- Levels implement `ILevel::buildScript()` to produce a `LevelScript` `{patterns, spawns, obstacles}`.
- `LevelFactory::makeLevel(levelId)` instantiates the right `ILevel` (currently `Level1`), and `SpawnConfig::buildSpawnSetupForLevel` feeds the spawners.

### Files to know
- `server/include/server/ILevel.hpp` : interface for levels.
- `server/include/levels/Level1.hpp` / `server/src/levels/Level1.cpp` : concrete level, offsets, shooter ratio, formation sizes.
- `server/include/server/WaveLibrary.hpp` / `server/src/WaveLibrary.cpp` : reusable wave builders (`line`, `stagger`, `triangle`, `serpent`, `cross`, `offsetWave`).
- `server/src/systems/MonsterSpawnSystem.cpp` : applies `SpawnEvent` into entities (sets position, scale, movement pattern, health, optional shooting component).
- `server/include/server/ObstacleLibrary.hpp` : helpers to place obstacles at a y pixel, stuck to top, or stuck to bottom.
- `server/src/systems/ObstacleSpawnSystem.cpp` : applies `ObstacleSpawn` into static entities (position, hitbox, health, render type).
- `server/src/SpawnConfig.cpp` + `server/src/levels/LevelFactory.cpp` : glue to pick the level script.

## Level1 specifics
- Patterns: 6 slots mixing linear/sine/zigzag with different speeds and amplitudes to keep waves varied (slow drift,
  wide sine, fast zigzag, heavy zigzag, etc.).
- Waves (spawns span ~48s → roughly a 1 min level once travel time is included):
  - 1.0s: stagger line x6, scale 1.6, shooters every 3rd.
  - 5.5s: triangle x9, scale 1.9, shooters every 4th.
  - 9.0s: serpent x8, scale 1.8, shooters every 3rd.
  - 13.5s: cross x13, HP=2, scale 2.2, shooters every 2nd.
  - 18.0s: top line x4, scale 1.7, shooters every 3rd.
  - 18.8s: bottom line x4, scale 1.7, shooters every 3rd.
  - 22.5s: zigzag triangle x9, HP=2, scale 2.2, shooters every 3rd.
  - 29.0s: heavy serpent x8, HP=2, scale 2.0, shooters every 2nd.
  - 35.5s: fast stagger x8, scale 1.8, shooters every 2nd.
  - 42.0s: final cross x13, HP=3, scale 2.5, shooters every 2nd.
  - 48.0s: cleanup line x6, scale 1.6, shooters every 3rd.
- Obstacles: 10 blockers mixing anchors (3 top / 3 bottom / 4 absolute) from t=3.0s to t=40.0s, scaled between 1.4
  and 2.3, to carve corridors and force pathing changes mid-level.

## Extending with a new level
1) Create `LevelN.hpp/cpp` in `server/include/levels` and `server/src/levels`, inherit `ILevel`, implement `buildScript()`.
2) Compose waves using `WaveLibrary` builders and add obstacles with `ObstacleLibrary` helpers.
3) Add the new level in `LevelFactory::makeLevel` and call `buildSpawnSetupForLevel(<id>)` from `ServerRunner`.
4) Optionally add tests to validate timings, shooter ratios, obstacle placement, and health/scale expectations.
