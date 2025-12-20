# Levels and Waves (JSON)

## Overview

Levels are authored in JSON and loaded by `LevelLoader` at server start. Runtime flow:

- `LevelLoader` parses JSON into `LevelData`.
- `LevelDirector` runs the segment state machine, evaluates triggers, and emits events.
- `LevelSpawnSystem` consumes spawn events and creates entities.
- `ServerRunner` forwards LevelEvent packets for scroll/background/music/camera/gates.

This replaces the legacy C++ `ILevel`/`LevelFactory`/`SpawnConfig` flow. The old scripts remain only for migration.

## Files to know

- `server/include/server/LevelData.hpp` : data model used by the loader and runtime.
- `server/src/LevelLoader.cpp` : JSON parsing and validation.
- `server/src/LevelDirector.cpp` : segment timeline and trigger evaluation.
- `server/src/LevelSpawnSystem.cpp` : enemy/obstacle/boss spawners.
- `shared/include/network/LevelEventData.hpp` : runtime event network payload.
- `docs/server/level-json-format.md` : JSON format and rules.
- `docs/server/level-templates.md` : template and override semantics.
- `docs/server/level-timeline.md` : trigger evaluation and repeat rules.

## Waves

Waves are spawned by `spawn_wave` events. Each wave references:

- `enemy` template id
- `patternId` for movement
- wave type: `line`, `stagger`, `triangle`, `serpent`, `cross`
- geometry fields for the chosen type
- optional overrides: `health`, `scale`, `shootingEnabled`

## Obstacles

`spawn_obstacle` events create scrolling blockers from obstacle templates.
Anchors:

- `absolute` uses `y`
- `top` or `bottom` uses `margin`

Speed, scale, and health can be overridden per event.

## Bosses and checkpoints

- Bosses spawn via `spawn_boss` events and are driven by boss phases.
- Checkpoints are events that capture respawn state and reset rules.

See `docs/server/level-boss-spec.md` and `docs/server/level-checkpoints.md`.

## Adding a level

1) Create a JSON file in `server/assets/levels/`.
2) Add the entry in `server/assets/levels/registry.json`.
3) Start the server; `LevelLoader` will load the new level.
