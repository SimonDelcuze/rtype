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
- Player bounds events (`set_player_bounds`, `clear_player_bounds`) apply server-side only.

## Checkpoints

Checkpoint events only mark ids for trigger evaluation. No snapshot is stored.

On player death, the server starts a respawn timer. When it expires, the player respawns on the left side of the screen
with base loadout and the level continues without reset.

## Error Handling

`LevelLoader` validates schema and semantics. If loading fails, the server logs the error and refuses to start.

## Related

- `docs/server/level-json-format.md`
- `docs/server/level-events.md`
- `docs/server/level-loader-design.md`
- `docs/server/level-director.md`
- `docs/server/level-spawn-integration.md`
- `docs/server/level-checkpoints.md`
