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
- Patterns: linear, sine, zigzag.
- Waves:
  - Wave1 (triangle) @ t=1.0s, HP=1, scale=2.0, shooters every 4th mob.
  - Wave2 (serpent) @ t=3.0s, HP=1, scale=2.0, shooters every 4th mob.
  - Wave3 (cross) @ t=6.0s, HP=1, scale=2.0, shooters every 4th mob.
- Shooter ratio is configurable per wave via `waveXShooterModulo_` (1 out of N keeps `shootingEnabled=true`).
- Offsets are configurable (`waveXOffset_`) to space waves.

## Extending with a new level
1) Create `LevelN.hpp/cpp` in `server/include/levels` and `server/src/levels`, inherit `ILevel`, implement `buildScript()`.
2) Compose waves using `WaveLibrary` builders and add obstacles with `ObstacleLibrary` helpers.
3) Add the new level in `LevelFactory::makeLevel` and call `buildSpawnSetupForLevel(<id>)` from `ServerRunner`.
4) Optionally add tests to validate timings, shooter ratios, obstacle placement, and health/scale expectations.
