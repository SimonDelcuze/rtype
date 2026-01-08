# LevelDirector Runtime Design

This document defines the LevelDirector state machine and event dispatch flow.
It is the runtime component that turns `LevelData` into gameplay events.

## Responsibilities

- Track the current segment and its clocks (time + distance).
- Evaluate triggers and dispatch level events.
- Apply local effects required for deterministic evaluation (scroll, checkpoints).
- Manage boss phase transitions and boss events.

## State Machine

The LevelDirector holds:

- `segmentIndex`
- `segmentTime` (real time since segment start)
- `segmentDistance` (scroll distance since segment start)
- `activeScroll` (current scroll mode/speed)

Segment transitions happen when the segment `exit` trigger becomes true.
On transition:

- `segmentTime` resets to 0
- `segmentDistance` resets to 0
- `activeScroll` resets to the segment scroll
- segment events are reset

## Event Dispatch

For each tick:

1) Advance time and distance using the current scroll speed.
2) Evaluate all segment events in order.
3) Evaluate boss phases and boss events.
4) If the exit trigger is true, transition to the next segment.

Dispatched events are queued and consumed by the caller.

## Trigger Semantics

Triggers are evaluated against a context:

- `time`: segment time (or phase time for boss events)
- `distance`: segment distance (or phase distance)
- `spawn_dead`: entity mapped to spawnId is not alive
- `boss_dead`: boss entity not alive
- `enemy_count_at_most`: current enemy count in registry
- `checkpoint_reached`: checkpoint id already recorded
- `hp_below`: boss health below threshold
- `player_in_zone`: player inside bounds
- `players_ready`: all alive players confirmed
- `all_of` / `any_of`: recursive evaluation

## Boss Phases

- Each boss has ordered phases.
- A phase is activated when its trigger is true.
- On activation, phase events use a local phase clock (time/distance).
- Boss `onDeath` events fire once when the boss entity dies.

## Scroll and Checkpoints

- `set_scroll` events update `activeScroll` immediately.
- `checkpoint` events mark checkpoints as reached.
- The server snapshots LevelDirector runtime when a checkpoint event fires.

These effects are applied inside the LevelDirector to keep triggers deterministic.

## Integration

The LevelDirector does not spawn entities directly.
It emits `DispatchedEvent` entries, which the server later converts into:

- Monster spawns
- Obstacle spawns
- Boss spawns
- Network level events (background/music/scroll/gates)
