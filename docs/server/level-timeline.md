# Level Timeline and Triggers

This document defines the runtime timeline model and trigger semantics used by the level system.
It describes how events are scheduled and how triggers are evaluated.

## Timeline Model

Each segment has its own timeline and two clocks:

- `realTime`: always increases with server tick time (seconds).
- `scrollTime`: increases based on scroll distance (world units).

Events can be triggered by either clock or by world state.

## Event Evaluation Order

On every server tick:

1) Update `realTime` and `scrollTime` for the current segment.
2) Evaluate triggers in the following order:
   - time/distance triggers
   - entity state triggers (death, count, hp)
   - composite triggers (all_of, any_of)
3) Dispatch events whose triggers become true.
4) Apply event effects in a stable order (sorted by event id and list order).

This order is required for deterministic replay at checkpoints.

## Trigger Types (v1)

### Time

`time`: fires when `realTime >= trigger.time`.

Use for: spawn timing, boss intro sequences, scripted pacing.

### Distance

`distance`: fires when `scrollTime >= trigger.distance`.

Use for: checkpoints, gates, segment transitions that depend on movement.

### Entity State

`spawn_dead`: fires when a specific spawn id is destroyed.
`boss_dead`: fires when a boss id is destroyed.
`enemy_count_at_most`: fires when enemies in the segment are <= N.
`hp_below`: fires when boss hp is below a threshold.

Entity state is evaluated on the authoritative server entity registry.

### Checkpoint

`checkpoint_reached`: fires when a checkpoint id has been recorded.

This is primarily for sequencing post-checkpoint events.

### Composite

`all_of`: fires when all child triggers are true.
`any_of`: fires when any child trigger is true.

Composite triggers are evaluated after leaf triggers.

## Trigger Edge Cases

- A trigger fires once unless the event has a `repeat` block.
- Triggers that are already true at segment start should fire on the first tick.
- If a trigger depends on a spawn id that never existed, the loader must reject the level.

## Repeat Semantics

Events can repeat using a `repeat` block:

- `count`: repeat N times.
- `interval`: time between repeats (seconds, realTime).
- `until`: stop when a trigger becomes true.

Repeat evaluation uses `realTime` and is independent from `scrollTime`.

## Checkpoint Determinism

On checkpoint reset:

- The segment is rewound to the checkpoint `realTime` and `scrollTime`.
- Events before the checkpoint are considered already fired.
- Entity state is rebuilt by replaying events from the checkpoint.

This requires event order and trigger semantics to be stable.

## Recommended Usage

- Use `distance` for checkpoints and corridor gates.
- Use `time` for wave pacing.
- Use `spawn_dead` or `boss_dead` for segment exits.
