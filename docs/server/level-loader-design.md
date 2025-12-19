# LevelLoader Design

This document describes the design of the LevelLoader responsible for parsing,
validating, and registering JSON levels.

## Goals

- JSON is mandatory for all levels.
- Fail fast with clear errors when data is invalid.
- Provide deterministic results (same input -> same LevelData).
- Support a registry of available levels for server selection.

## Responsibilities

The LevelLoader must:

1) Locate the level file (by registry or naming convention).
2) Read the file contents.
3) Parse JSON.
4) Validate against the schema.
5) Validate semantics and references.
6) Build `LevelData` for runtime use.

## Registry of Levels

To avoid hardcoded level ids, the server uses a registry file:

- `server/assets/levels/registry.json`

Example structure:

```
{
  "schemaVersion": 1,
  "levels": [
    { "id": 1, "path": "level_01.json", "name": "Stage 1" },
    { "id": 2, "path": "level_02.json", "name": "Stage 2" }
  ]
}
```

Rules:

- `id` must be unique and > 0.
- `path` is relative to `server/assets/levels/`.
- The registry is optional for development, but required for production.
- If no registry exists, fallback to `level_<id>.json`, then `level_<id:02>.json`.

## Parsing and Validation Pipeline

The loader performs these steps in order:

1) File read:
   - If the file is missing, return a fatal error.
   - If the file is unreadable, return a fatal error with OS details.

2) JSON parse:
   - Invalid JSON is rejected with line/column information.

3) Schema validation:
   - The JSON must match `docs/server/level-json-schema-v1.json`.
   - `schemaVersion` must be exactly `1`.

4) Semantic validation:
   - All ids are unique (patterns, segments, checkpoints, bosses, templates).
   - All references exist (patternId, enemy template, obstacle template, bossId).
   - Polygon colliders have >= 3 points and finite values.
   - Scales are finite and > 0.
   - Anchor rules are respected (absolute requires y, top/bottom require margin).
   - Boss phases are ordered and triggers are reachable.

5) Runtime build:
   - Convert patterns to MovementComponent defaults.
   - Convert templates to cached hitbox/collider objects.
   - Convert segments/events to an internal timeline format.

## Error Model

Errors should be categorized and provide context:

- `FileNotFound`: missing JSON file or registry.
- `FileReadError`: IO or permissions error.
- `JsonParseError`: invalid JSON syntax.
- `SchemaError`: failed schema validation.
- `SemanticError`: reference or logic errors.

Each error includes:

- `levelId` (if known).
- `path` of the file.
- a JSON pointer (when relevant).
- a human-readable message.

The loader must stop on the first fatal error and refuse to start the level.

## Determinism Notes

- The order of events must remain stable after parsing.
- Sorting by JSON order must be preserved.
- The seed from the level file must be used for any randomness.

## Output

The loader returns a `LevelData` struct:

- `LevelDefinition` data for LevelInit (background/music/archetypes).
- `LevelScript` data for spawners.
- `SegmentTimeline` data for the LevelDirector.

The exact structs will be defined in runtime issues.
