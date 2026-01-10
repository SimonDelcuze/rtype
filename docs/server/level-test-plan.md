# Level System Test Plan

This plan covers validation, runtime behavior, and regressions for the JSON level system.

## Goals

- Catch invalid JSON/schema/semantic errors early.
- Verify respawn delay and position.
- Ensure boss phases continue without reset on player death.
- Validate scroll changes (constant, stopped, curve) in runtime events.

## Test Data

Create a minimal set of JSON fixtures under `server/assets/levels/test/` (or a test data folder):

- `invalid_schema_missing_fields.json`
- `invalid_schema_bad_types.json`
- `invalid_semantic_unknown_ids.json`
- `invalid_semantic_duplicate_ids.json`
- `respawn_basic.json`
- `scroll_constant.json`
- `scroll_stop_resume.json`
- `scroll_curve.json`

## JSON Invalids (Schema + Semantic)

### Parse failures

- Malformed JSON (missing braces, trailing commas)
- Invalid UTF-8 sequences in source file

Expected: loader returns `LevelLoadError` with `JsonParseError`.

### Schema failures

- Missing required fields: `schemaVersion`, `levelId`, `meta`, `archetypes`, `patterns`, `templates`, `segments`
- Wrong type: strings where numbers are expected (ex: `speedX: "fast"`)
- Missing required fields in events (ex: `spawn_wave` without `wave`)
- Invalid enum values (ex: `scroll.mode = "warp"`)

Expected: loader returns `LevelLoadError` with `SchemaError`, path and jsonPointer set.

### Semantic failures

- Duplicate ids (patterns/segments/templates/checkpoints/bosses)
- Unknown reference ids (`patternId`, `enemy`, `obstacle`, `bossId`, `gateId`)
- Invalid collider shapes (polygon with <3 points, circle without radius)
- Scroll curve not starting at time 0 or unsorted keyframes

Expected: loader returns `SemanticError` with clear message and path.

## Respawn Behavior

### Respawn timing and position

- Build a level with waves and obstacles.
- Die while the level is running.

Expected on respawn:

- Player respawns after the configured delay.
- Player respawns on the left side of the screen with base loadout and invincibility.
- The level timeline, spawns, and bosses continue without reset.

## Boss Continuity

### Boss room death

- Trigger a boss spawn.
- Die and respawn during the boss fight.

Expected:

- Boss remains alive with current state.
- Boss phase progression continues without reset.

## Scroll Variable

### Constant scroll

- Use `set_scroll` with constant speed.

Expected:

- Client background speed reflects server scroll.
- Distance-based triggers align with expected segment distance.

### Stop and resume

- `set_scroll` to stopped during a boss room, then resume.

Expected:

- Segment distance stops increasing while stopped.
- Distance triggers do not fire during stop.
- Resume restores speed and triggers fire correctly.

### Curve scroll

- Use a curve with multiple keyframes.

Expected:

- Client receives `set_scroll` and adjusts speed over time.
- Segment distance integrates speed changes correctly.

## Suggested Automation

- Unit tests for LevelLoader to validate schema/semantic errors.
- Integration tests that drive LevelDirector/LevelSpawnSystem with a deterministic seed.
- Snapshot tests for LevelEvent parsing (client) to validate scroll/background/music/camera/gate payloads.

## Regression Checklist

- Loader error reporting paths are stable.
- Checkpoint reset clears entities and does not leak spawn groups.
- Boss state is deterministic across reset.
- Scroll changes do not desync client background speed.
