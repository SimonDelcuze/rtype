# Level Event System

This system consumes `LevelEvent` messages from the server and applies runtime changes on the client.

## Responsibilities

- Update background scroll speed when `set_scroll` events arrive.
- Swap backgrounds on `set_background` events.
- Play or switch music on `set_music` events.
- Record camera bounds and gate states for future systems.

## Behavior

- `set_scroll` updates an internal scroll clock and recomputes the background speed every frame.
- `set_background` swaps textures on existing background entities or creates one if missing.
- `set_music` loads the music asset from the manifest and plays it in a loop.
- `set_camera_bounds` stores bounds; no camera system consumes it yet.
- `gate_open` / `gate_close` are stored by id for future gate handling.

## Integration

The system runs after `NetworkMessageSystem` and `LevelInitSystem`, and before `BackgroundScrollSystem`.

## Migration / Tests / TODO

- Migration: send initial `set_scroll` after LevelInit; optional `set_music` if music should start immediately.
- Tests: parser validity for each event type, CRC rejection, background swap without entities.
- TODO: camera bounds consumption in a camera system, gate entity visuals/logic.
