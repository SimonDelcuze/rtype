# Level Templates and Prototypes

This document specifies how templates are defined and resolved for enemies, obstacles, and bosses.
Templates are mandatory and live inside the level JSON under `templates` and `bosses`.

## Goals

- Avoid duplication of hitboxes, colliders, and shooting data.
- Keep spawn events clean and focused on timing/placement.
- Allow future content (boss phases, gates, special rooms) without changing core format.

## Template Collections

Templates are grouped by type:

- `templates.hitboxes`: reusable hitbox definitions.
- `templates.colliders`: reusable collider definitions.
- `templates.enemies`: enemy prototypes.
- `templates.obstacles`: obstacle prototypes.
- `bosses`: boss prototypes (specialized enemies with phases).

All template ids are dictionary keys and must be unique per collection.

## Hitbox Templates

Hitboxes are lightweight axis-aligned boxes used for damage checks and as a fallback collider.

Fields:

- `width`, `height` (required, >= 0)
- `offsetX`, `offsetY` (optional)
- `active` (optional, default true)

## Collider Templates

Colliders are used by the collision system. If a collider is present, it takes priority over the hitbox.

Supported shapes:

- `box` (width/height required)
- `circle` (radius required)
- `polygon` (points required, >= 3)

All collider points are defined in local space and will be transformed by the entity scale.

## Shooting Block

Shooting is optional and maps directly to `EnemyShootingComponent::create`.

Fields:

- `interval` (seconds between shots)
- `speed` (projectile speed)
- `damage` (int)
- `lifetime` (seconds)

If absent, the entity does not shoot (unless overridden by a spawn event).

## Enemy Templates

Enemy templates define reusable defaults for spawns in waves.

Fields:

- `typeId` (required, snapshot entity type)
- `hitbox` (id from templates.hitboxes)
- `collider` (id from templates.colliders)
- `health` (default HP)
- `scale` ([x, y])
- `shooting` (optional)

Waves can override `health`, `scale`, and `shootingEnabled`.

## Obstacle Templates

Obstacle templates define reusable blockers with anchors and speeds.

Fields:

- `typeId` (required)
- `hitbox` (id)
- `collider` (id)
- `health` (default HP)
- `anchor` (top, bottom, absolute)
- `margin` (used for top/bottom anchors)
- `speedX`, `speedY`
- `scale` ([x, y])

Spawn events can override `anchor`, `margin`, `health`, `scale`, `speedX`, and `speedY`.

## Boss Templates

Bosses are defined in the `bosses` section and are referenced by `spawn_boss`.

Fields:

- `typeId` (required)
- `hitbox` (id)
- `collider` (id)
- `health` (default HP)
- `scale` ([x, y])
- `patternId` (optional movement pattern id)
- `shooting` (optional, same schema as enemy shooting)
- `phases` (optional array)
- `onDeath` (optional events)

Phases are ordered and triggered by a condition (time, hp, or composite trigger).
They can spawn waves, toggle gates, or change scroll behavior.

## Resolution Rules and Overrides

1) A spawn event selects a template by id.
2) The template provides defaults (hitbox/collider/health/scale/shooting).
3) The spawn event overrides allowed fields.
4) If a collider is missing or invalid, the hitbox is used for collision.

Priority is always: event override > template value > safe default.

## Validation Checklist (Loader)

The loader must reject a level if:

- any referenced template id does not exist.
- any collider shape is missing required data.
- any scale is non-finite or <= 0.
- any polygon has < 3 points.

These rules are required for deterministic behavior and safe collision checks.
