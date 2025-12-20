# Level Runtime Overview

This document summarizes how the server executes a JSON level at runtime.

## Data Flow

1) `LevelLoader` parses JSON into `LevelData`.
2) `LevelDirector` runs the segment state machine and evaluates triggers.
3) `LevelSpawnSystem` consumes spawn events and creates entities.
4) `ServerRunner` sends `LevelEvent` packets for scroll/background/music/camera/gates.

## Segment State Machine

- Enter segment: reset segment clocks, apply initial scroll, build runtime event list.
- Each tick:
  - Advance segment time and distance.
  - Evaluate triggers in deterministic order.
  - Dispatch events whose triggers are satisfied.
- Exit trigger moves to the next segment; the final segment marks the level finished.

## Event Dispatch

Events are dispatched by `LevelDirector` and split into two buckets:

- Spawn events (`spawn_wave`, `spawn_obstacle`, `spawn_boss`) go to `LevelSpawnSystem`.
- State/visual events (`set_scroll`, `set_background`, `set_music`, `set_camera_bounds`, `gate_open`, `gate_close`)
  are forwarded to clients as `LevelEvent` messages.

## Checkpoints

When a checkpoint event fires, the server captures a snapshot:

- Segment index, time, distance, and active scroll.
- Fired event state and repeat counters.
- Spawn group state and pending spawns.
- Boss alive/dead status.
- Set of reached checkpoints.

On player death, the server resets to the snapshot, clears non-player entities, and respawns players with base loadout.

## Error Handling

`LevelLoader` validates schema and semantics. If loading fails, the server logs the error and refuses to start.

## Related

- `docs/server/level-json-format.md`
- `docs/server/level-events.md`
- `docs/server/level-loader-design.md`
- `docs/server/level-director.md`
- `docs/server/level-spawn-integration.md`
- `docs/server/level-checkpoints.md`
