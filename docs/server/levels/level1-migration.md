# Level1 Migration Plan (C++ to JSON)

Goal: move Level1 from hardcoded C++ (Level1.cpp) to a JSON file that matches current gameplay.

## Source Data (from Level1.cpp)

Patterns (index -> JSON id):
- 0: linear speed 140
- 1: sine speed 160, amplitude 120, frequency 0.5, phase 0
- 2: zigzag speed 150, amplitude 90, frequency 1.0
- 3: sine speed 130, amplitude 180, frequency 0.6, phase 0.5
- 4: linear speed 200
- 5: zigzag speed 120, amplitude 150, frequency 0.85

Enemy template:
- hitbox: 50x50, offset 0, active true
- collider: box (same size as hitbox)
- shooting: interval 1.4, speed 320, damage 6, lifetime 3.2
- default typeId: 2 (mob1)

Waves (time offsets are segment time):
1) time 1.0: stagger, spawnX 1220, startY 140, deltaY 55, count 6, spacing 0.55, pattern 0, hp 1, scale 1.6, shooterModulo 3
2) time 5.5: triangle, spawnX 1220, apexY 260, rowHeight 42, layers 3, step 38, pattern 1, hp 1, scale 1.9, shooterModulo 4
3) time 9.0: serpent, spawnX 1220, startY 110, stepY 30, count 8, amplitudeX 110, stepTime 0.4, pattern 2, hp 1, scale 1.8, shooterModulo 3
4) time 13.5: cross, centerX 1200, centerY 320, step 48, armLength 3, pattern 3, hp 2, scale 2.2, shooterModulo 2
5) time 18.0: line, spawnX 1220, startY 80, deltaY 90, count 4, pattern 4, hp 1, scale 1.7, shooterModulo 3
6) time 18.8: line, spawnX 1220, startY 520, deltaY -90, count 4, pattern 4, hp 1, scale 1.7, shooterModulo 3
7) time 22.5: triangle, spawnX 1190, apexY 200, rowHeight 36, layers 3, step 34, pattern 5, hp 2, scale 2.2, shooterModulo 3
8) time 29.0: serpent, spawnX 1220, startY 160, stepY 28, count 8, amplitudeX 140, stepTime 0.35, pattern 3, hp 2, scale 2.0, shooterModulo 2
9) time 35.5: stagger, spawnX 1220, startY 340, deltaY -35, count 8, spacing 0.32, pattern 4, hp 1, scale 1.8, shooterModulo 2
10) time 42.0: cross, centerX 1180, centerY 300, step 44, armLength 3, pattern 5, hp 3, scale 2.5, shooterModulo 2
11) time 48.0: line, spawnX 1220, startY 120, deltaY 70, count 6, pattern 0, hp 1, scale 1.6, shooterModulo 3

Obstacles (spawnX = 1480):
- t=3.0: top, hitbox top, health 28, margin 0, speedX -45, typeId 9, scale 1.6
- t=6.5: bottom, hitbox bottom, health 32, margin 0, speedX -55, typeId 11, scale 1.9
- t=9.5: absolute y=260, hitbox mid, health 30, speedX -60, typeId 10, scale 1.4
- t=13.2: top, hitbox top, health 40, margin 0, speedX -50, typeId 9, scale 2.2
- t=15.0: bottom, hitbox bottom, health 40, margin 0, speedX -50, typeId 11, scale 2.0
- t=21.0: absolute y=360, hitbox mid, health 28, speedX -65, typeId 10, scale 1.6
- t=25.0: top, hitbox top, health 35, margin 0, speedX -55, typeId 9, scale 2.3
- t=28.0: bottom, hitbox bottom, health 35, margin 0, speedX -55, typeId 11, scale 2.3
- t=34.0: absolute y=200, hitbox mid, health 30, speedX -70, typeId 10, scale 1.8
- t=40.0: absolute y=430, hitbox mid, health 45, speedX -60, typeId 10, scale 2.0

Obstacle colliders:
- topHull, midHull, bottomHull polygon points are defined in Level1.cpp and must be copied into JSON collider templates.

## Migration Steps

1) Create JSON file `server/assets/levels/level_01.json` based on v1 schema.
2) Define patterns with ids that match the 6 MovementComponent configs above.
3) Define templates:
   - hitboxes: enemy + obstacle top/mid/bottom
   - colliders: enemy box + obstacle polygons (top/mid/bottom)
   - enemies: one template using typeId=2 and the shooting block above
   - obstacles: three templates using typeId 9/10/11, anchor top/absolute/bottom
4) Build a single segment:
   - scroll: constant speedX -50 (matches current background scroll)
   - events: 11 spawn_wave events with the timings above
   - events: 10 spawn_obstacle events with the timings above
   - exit trigger: time >= lastSpawn + buffer (ex: 60.0) or enemy_count_at_most + time
5) Shooter modulo handling:
   - Option A: split waves into multiple events (shooters vs non-shooters) to preserve ratio
   - Option B: extend JSON schema with a shooting modulo field and implement it in LevelSpawnSystem
6) Register the level in `server/assets/levels/registry.json` (or rely on level_01.json fallback).
7) Update LevelInit to use JSON meta/archetypes so background/music match the JSON file.
8) Keep Level1.cpp until parity is confirmed, then remove or archive it.

## Parity Checklist

- [ ] All 6 patterns match speed/amplitude/frequency/phase values
- [ ] Enemy template uses typeId 2, hitbox 50x50, collider box, shooting 1.4/320/6/3.2
- [ ] 11 waves spawn at the same times with the same params and scales
- [ ] Shooter ratio per wave matches the C++ modulo behavior
- [ ] Obstacles: count 10, times match, anchors match (top=3, bottom=3, absolute=4)
- [ ] Obstacle speeds and scales match per spawn
- [ ] Obstacle polygon colliders match the C++ point lists
- [ ] Scroll speed -50 constant across the segment
- [ ] Background/music ids match LevelInit assets
- [ ] No extra spawns before 1.0s or after 48.0s (last wave offset)

## Sub-issues (Migration / Tests / TODO)

Migration:
- JSON authoring for Level1 (patterns, templates, segments)
- Shooter modulo strategy (split waves or extend schema)
- Registry entry and LevelInit metadata wiring

Tests:
- Add LevelLoader tests that load level_01.json and assert wave/obstacle counts and timings
- Replace Level1Tests/LevelFactoryTests with JSON-based parity checks

TODO:
- Remove Level1.cpp and old LevelFactory path once parity is validated
- Document final JSON level in `docs/server/level-example-v1.json` if needed
