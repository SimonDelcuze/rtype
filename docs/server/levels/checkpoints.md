# Checkpoint Runtime Rules

This document describes how checkpoints are handled on the server.

Checkpoint resets are not part of the runtime anymore. Checkpoint events only mark ids for trigger evaluation.

## Respawn

When a player respawns (after the death timer):

- the level timeline continues without rewind
- the player respawns on the left side of the screen with base loadout
- non-player entities are not reset
