# Checkpoint Runtime Rules

This document describes how checkpoint reset is handled on the server.

## Snapshot

When a `checkpoint` event fires, the server stores:

- segment index, segment time, segment distance
- active scroll settings
- active player bounds (if set)
- segment event runtime state (fired flags, repeat counters)
- reached checkpoint ids
- spawn groups marked as spawned
- boss status at the checkpoint (alive or dead)
- LevelSpawnSystem state (time and pending enemy spawns, boss spawn settings)
- respawn position from the checkpoint event

## Reset Flow

When a player respawns (after the death timer):

- destroy all non-player entities
- restore LevelDirector state (timeline, scroll, checkpoints)
- restore LevelSpawnSystem state (pending spawns)
- respawn bosses that were alive at the checkpoint with full health and phase 0
- keep bosses that were dead at the checkpoint dead (no onDeath replay)
- respawn all players at the checkpoint respawn position with base loadout
- if no checkpoint exists, reset to level start and use the default respawn position

## Notes

- `spawn_dead` triggers for groups spawned before the checkpoint evaluate as dead after reset.
- checkpoint reset applies to the whole world, not a single player.
