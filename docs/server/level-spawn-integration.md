# Level Spawn Integration

This document describes how LevelDirector events are converted into monster and obstacle spawns.

## Components

- `LevelDirector` emits `DispatchedEvent` entries.
- `LevelSpawnSystem` consumes these events and spawns entities using templates.

## Event to Spawn Mapping

### spawn_wave

1) Resolve `patternId` to a `MovementComponent`.
2) Resolve `enemy` to an `EnemyTemplate`.
3) Build spawn instances using the wave type:
   - `line`, `triangle`, `cross` spawn all at once.
   - `stagger` uses `spacing` time offsets.
   - `serpent` uses `stepTime` time offsets.
4) Each instance is scheduled with a time offset and spawned when due.

Wave overrides:

- `health` and `scale` override template defaults.
- `shootingEnabled=false` disables shooting even if the template has it.

### spawn_obstacle

1) Resolve `obstacle` to an `ObstacleTemplate`.
2) Merge overrides from the event (anchor, margin, scale, speed, health).
3) Compute final Y using anchor rules (top/bottom/absolute).
4) Spawn immediately.

### spawn_boss

Bosses are spawned like enemies using `BossDefinition` data:

- Tag: Enemy
- Health, hitbox, collider, scale from boss template
- Render type from boss `typeId`

Boss registration is handled so boss triggers can evaluate.

## Spawn IDs and Triggers

- `spawn_wave` uses the event `id` as a spawn group id.
- `spawn_obstacle` uses `spawnId` if set, otherwise event `id`.
- `spawn_boss` uses `spawnId` if set, otherwise event `id`.

Spawn groups can contain multiple entities (e.g. waves).
`spawn_dead` triggers become true when all entities in the group are dead.

## Determinism

- The spawn scheduler uses the same tick delta as the LevelDirector.
- Delayed spawns are scheduled using absolute time offsets.
- Spawn order is stable and deterministic.
