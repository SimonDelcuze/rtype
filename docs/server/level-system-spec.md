# R-Type Level System Specification (Data-Driven)

## Goals

- Define a level system that can reproduce classic R-Type behavior.
- Make level creation 100% JSON-driven (no C++ fallback).
- Support future content: bosses, phases, checkpoints, gates, and scripted rooms.
- Keep the server authoritative and deterministic for multiplayer.

## Terms

- Level: Full stage, composed of ordered segments.
- Segment: A bounded part of a level with its own scroll behavior and events.
- Event: A time or trigger-based action (spawn, scroll change, boss start, gate).
- Trigger: Condition that activates an event (time, distance, entity state).
- Boss Room: Segment that can override scroll and lock progress.
- Gate: Blocking element controlling player progress during scripted moments.
- Checkpoint: Explicit point where player can restart after death.

## Core Design

- The server owns the level timeline and is the single source of truth.
- JSON is mandatory for every level and is validated at load time.
- The level system is deterministic:
  - All randomness must use the level seed.
  - Event order is stable and reproducible.
- The level system must be compatible with current networking:
  - `LevelInit` still carries archetypes and asset ids.
  - New runtime events use a dedicated LevelEvent message (future issue).

## Segment Model

Each segment defines:

- `id`: unique string.
- `scroll`: scroll profile for the segment (speed and optional curve).
- `events`: ordered list of events.
- `exit`: trigger that transitions to the next segment.
- `checkpoints`: explicit events inside the segment.
- `bossRoom`: optional flag that enables boss-specific behavior.

Segments allow:

- boss rooms with variable scroll (0, constant, or dynamic).
- corridor gates that open/close on triggers.
- custom camera bounds or background overrides.

## Trigger Model

Supported trigger types (initial spec):

- `time`: time since segment start (seconds).
- `distance`: scroll distance traveled since segment start (world units).
- `entity_dead`: entity or boss id destroyed.
- `entity_count`: remaining enemies <= N.
- `hp_below`: boss hp below threshold.
- `checkpoint`: explicit event in timeline.

Triggers must be evaluated in a stable order every tick.

## Scroll Model

Scroll is a segment-level setting and can be:

- constant speed (default classic R-Type).
- piecewise or curve-based (slowdown, stop, resume).
- disabled (boss room hold).

The level system tracks:

- `scrollPos`: total horizontal offset since level start.
- `segmentScrollPos`: offset since segment start.

This allows triggers based on distance, not only time.

## Checkpoint Rules (R-Type)

Checkpoints are explicit and placed at precise positions.

When a checkpoint is reached, the server stores a `CheckpointState`:

- current segment id and event index.
- scroll position (global and segment).
- level RNG seed and event counters.
- player respawn position.
- boss room state (if in or after a boss segment).

On player death:

- player loses all power-ups and respawns at base loadout.
- the world is reset to the last checkpoint.
- all enemies and obstacles after the checkpoint are reset.
- any active boss is reset to its initial state.

This matches classic R-Type behavior.

## Boss System (Future-Proof)

Bosses are treated as special entities with:

- explicit spawn event.
- phase triggers (hp, time, or custom).
- optional invulnerability windows.
- optional minion spawns.

Boss rooms can:

- lock scroll (stop or slow).
- close gates on entry, open on boss death.

Boss death or phase changes are modeled as events and triggers.

## Event Types (Initial Spec)

Minimum event list to cover R-Type behavior:

- `spawn_wave`: spawn a wave from a pattern/template.
- `spawn_obstacle`: place a blocker with anchor/scale/type.
- `spawn_boss`: create boss entity by id.
- `set_scroll`: change scroll speed or curve.
- `set_background`: change background id.
- `set_music`: change music id.
- `set_camera_bounds`: override camera clamp.
- `gate_open` / `gate_close`: control segment gates.
- `checkpoint`: mark checkpoint at precise position.

Events can be extended without breaking compatibility.

## Determinism and Replay

- All events are driven by the server timeline.
- Random choices must use the level seed.
- On checkpoint reset, the timeline is rewound to the checkpoint and replayed.
- The resulting world state must be identical to the original pass.

## Versioning and Validation

- Every level JSON must include a `schemaVersion`.
- Loader validates required fields and types.
- Invalid files are rejected with clear errors.

## Notes on Existing Code

The current `Level1.cpp` flow is a static C++ script. The new system replaces it with a JSON-driven segment timeline while still producing the same `LevelInit` payloads and snapshots.

## Related Docs

- `docs/server/levels-and-waves.md`
- `docs/server/level-loader-design.md`
- `docs/server/level-director.md`
- `docs/server/level-spawn-integration.md`
- `docs/protocol/level-messages.md`
- `docs/client/level-init-system.md`
