# Level Assets Conventions

This document defines the conventions for `server/assets/levels/`.

## Folder

- All level JSON files live under `server/assets/levels/`.
- This path is hard-coded in `LevelLoader::levelsRoot()`.

## File Naming

Preferred:

- `level_<id:02>.json` (zero-padded, ex: `level_01.json`)

Accepted fallback:

- `level_<id>.json` (non-padded)

Notes:

- Keep file names ASCII and lowercase.
- Avoid spaces in file names.

## Registry

A registry is optional, but if it exists it is authoritative.

Path:

- `server/assets/levels/registry.json`

Format:
```
{
  "schemaVersion": 1,
  "levels": [
    { "id": 1, "path": "level_01.json", "name": "Stage 1" }
  ]
}
```

Rules:

- `path` is relative to `server/assets/levels/`.
- `id` must be unique.
- If the registry exists and the requested id is missing, load fails.
- If the registry does not exist, the loader falls back to naming rules.

## Recommended Workflow

- Start without a registry while prototyping a single level.
- Add `registry.json` once you have multiple levels or want custom file names.
- Keep level ids stable; avoid renaming files without updating the registry.
