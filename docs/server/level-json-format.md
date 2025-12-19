# Level JSON Format (v1)

This document defines the mandatory JSON format for all levels.
It is aligned with the R-Type level system spec and is validated against the schema:

- `docs/server/level-json-schema-v1.json`

## File Location and Naming

- Levels live under `server/assets/levels/`.
- File names follow `level_<id>.json` (example: `level_01.json`).

## Top-Level Structure

Required fields:

- `schemaVersion` (must be `1`)
- `levelId` (positive integer)
- `meta` (background/music)
- `archetypes` (LevelInit data)
- `patterns` (movement patterns)
- `templates` (hitbox/collider/enemy/obstacle)
- `segments` (ordered list of segment definitions)

Optional fields:

- `bosses` (boss definitions with phases and events)

Units:

- Time: seconds.
- Position: pixels in world space.
- Speed: pixels per second (negative X means leftward).

## Validation Rules (Semantic)

Some rules are not enforceable by JSON Schema and must be checked by the loader:

- All ids are unique (patterns, segments, checkpoints, bosses, templates).
- All references exist (patternId, enemy template, obstacle template, bossId).
- `segments` order defines gameplay order (no cycles).
- Scroll curve keyframes are sorted by time and start at `time = 0`.
- `spawn_obstacle`:
  - `anchor = absolute` requires `y`.
  - `anchor = top/bottom` requires `margin` (or use template default).
- `checkpoint` ids are unique across the level.
- Boss phases are evaluated in order; each phase trigger must be reachable.

## Patterns

Patterns define movement behavior and are referenced by `patternId` in waves.

Types:

- `linear`
- `zigzag`
- `sine`

## Templates

Templates provide reusable data blocks referenced by events:

- `hitboxes`: width/height/offset/active
- `colliders`: box/circle/polygon
- `enemies`: typeId + hitbox/collider + defaults
- `obstacles`: typeId + hitbox/collider + defaults

See `docs/server/level-templates.md` for semantics and override rules.

## Segments

Segments are evaluated in order. Each segment defines:

- `scroll` (constant/curve/stopped)
- `events` (spawn waves, obstacles, checkpoints, bosses)
- `exit` (trigger to move to next segment)

Boss rooms are just segments with special events and triggers.
Scroll can be stopped, slowed, or left running.

## Triggers

Supported triggers in v1:

- `time` (since segment start)
- `distance` (scroll distance since segment start)
- `spawn_dead` (a spawn id has been destroyed)
- `boss_dead` (boss id destroyed)
- `enemy_count_at_most`
- `checkpoint_reached`
- `hp_below` (boss hp threshold)
- `all_of` / `any_of` composite triggers

See `docs/server/level-timeline.md` for trigger semantics and evaluation order.

## Events

Supported event types in v1:

- `spawn_wave`
- `spawn_obstacle`
- `spawn_boss`
- `set_scroll`
- `set_background`
- `set_music`
- `set_camera_bounds`
- `gate_open`
- `gate_close`
- `checkpoint`

Events always include a `trigger`.

`spawnId` is optional on spawn events and can be used to reference the entity
in triggers (ex: `spawn_dead`) or gate events.

## Example (Complete)

See `docs/server/level-example-v1.json` for a full example.
