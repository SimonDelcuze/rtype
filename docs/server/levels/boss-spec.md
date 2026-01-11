# Boss Specification (v1)

This document defines the boss model for R-Type style levels.
It is data-driven and intended to be implemented later without changing the JSON format.

## Goals

- Support multi-phase bosses with scripted transitions.
- Allow gates and scroll control during boss rooms.
- Enable invulnerability windows and minion spawns.
- Keep behavior deterministic and checkpoint-safe.

## Boss Lifecycle

1) Boss is spawned by a `spawn_boss` event.
2) Boss enters phase 1 immediately (or on its phase trigger).
3) Phase transitions occur based on triggers (hp, time, events).
4) On death, `onDeath` events are executed.
5) If the player dies in the boss room, the boss is reset to initial state.

## Phases

Each boss can define an ordered list of phases.

Phase fields:

- `id` (string)
- `trigger` (when the phase becomes active)
- `events` (actions fired on phase entry)

Typical phase triggers:

- `hp_below` for classic R-Type phase changes.
- `time` for scripted intros.
- `spawn_dead` for weakpoints or parts.

Phases are evaluated in order. Once a phase is active, earlier phases do not re-activate.

## Gates

Boss rooms often include gates:

- `gate_close` when entering the room.
- `gate_open` on boss death.

Gates are controlled by events in the level segment and boss `onDeath`.
The gate entity itself can be modeled as an obstacle or a special entity type.

## Invulnerability

Boss invulnerability is modeled as events:

- `boss_invuln_on`
- `boss_invuln_off`

These events can be emitted in phases or as time-based triggers.
If not implemented, the default is vulnerable at all times.

## Movement and Shooting

Boss templates can optionally define:

- `patternId` (movement pattern from the level `patterns` list)
- `shooting` (same schema as enemy shooting)

This keeps boss behavior aligned with regular enemies while still supporting boss phases.

## Minion Spawns

Boss phases can spawn minions using standard `spawn_wave` events.

This keeps minion logic in the existing wave system and avoids boss-specific spawn code.

## Transitions and Scroll

Boss rooms are standard segments:

- Scroll can be constant, slowed, or stopped.
- Scroll changes are driven by `set_scroll` events.

This allows flexible R-Type behavior, including moving bosses.

## Checkpoint and Respawn

On player death in a boss room:

- The boss and timeline continue without reset.
- The player respawns on the left side of the screen with base loadout.

## Network Implications (Future)

Boss-related events should be sent to clients via LevelEvent:

- phase change notifications (optional)
- gate open/close
- scroll changes
- background/music changes

The server remains authoritative; clients only mirror state for visuals.
