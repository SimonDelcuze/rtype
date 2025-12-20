# Level Events

This document describes level events defined in JSON and how they are executed at runtime.

## Event Lifecycle

- Events live under `segments[].events`.
- Each event has `id`, `trigger`, and optional `repeat`.
- `LevelDirector` evaluates triggers and dispatches events in a deterministic order.

## Spawn Events

### spawn_wave

Creates a wave of enemies using:

- `enemy` template id
- `patternId` for movement
- wave type (`line`, `stagger`, `triangle`, `serpent`, `cross`)
- geometry fields for the chosen type

Optional overrides:

- `health`
- `scale`
- `shootingEnabled`

Use the event `id` as the spawn group id for later triggers (ex: `spawn_dead`).

### spawn_obstacle

Spawns a single obstacle using an obstacle template. Per-event overrides can change:

- anchor (`absolute`, `top`, `bottom`)
- `y` or `margin` depending on anchor
- `health`, `scale`, `speedX`, `speedY`

`spawnId` can override the event `id` used for spawn tracking.

### spawn_boss

Spawns a boss by `bossId` at a fixed position. The boss runtime is registered so phase triggers and `boss_dead`
events can resolve.

`spawnId` can override the event `id` used for spawn tracking.

## State and Visual Events

These events update server state and are also forwarded to clients as `LevelEvent` packets:

- `set_scroll` updates the active scroll settings (constant, stopped, curve).
- `set_background` swaps the background texture.
- `set_music` starts or changes music.
- `set_camera_bounds` updates camera clamps.
- `gate_open` / `gate_close` set gate state on the client.

## Checkpoint Event

`checkpoint` marks a checkpoint id and provides a respawn position. The server captures a checkpoint snapshot
after dispatching events for that tick.

## Network Mapping

Only state/visual events are sent to clients. Spawn events and checkpoints are server-only; clients see the results
through snapshots.

## Related

- `docs/server/level-json-format.md`
- `docs/server/level-timeline.md`
- `docs/server/level-director.md`
- `docs/protocol/level-messages.md`
- `docs/client/level-event-system.md`
