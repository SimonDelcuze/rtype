# Animation Manifest & Labels

The client loads `client/assets/animations.json` to cut spritesheets and choose animations without hardcoding.

## JSON structure

```json
{
  "animations": [
    { "id": "player_ship_row1", "frameTime": 0.5, "loop": true, "frames": [
      { "x": 0, "y": 0, "width": 34, "height": 18 },
      { "x": 33, "y": 0, "width": 34, "height": 18 }
    ]},
    { "id": "player_ship_row2", "frameTime": 0.5, "loop": true, "frames": [
      { "x": 0, "y": 18, "width": 34, "height": 18 },
      { "x": 33, "y": 18, "width": 34, "height": 18 }
    ]},
    { "id": "bullet_basic", "frameTime": 0.12, "loop": true, "frames": [
      { "x": 248, "y": 88, "width": 18, "height": 7 }
    ]},
    { "id": "bullet_charge_lvl1", "frameTime": 0.12, "loop": true, "frames": [
      { "x": 231, "y": 99, "width": 18, "height": 19 }
    ]}
  ],
  "labels": {
    "player_ship": {
      "row1": "player_ship_row1",
      "row2": "player_ship_row2",
      "player1": "player_ship_row1",
      "player2": "player_ship_row2"
    },
    "bullet": {
      "base": "bullet_basic",
      "charge": "bullet_charge_warmup",
      "charge1": "bullet_charge_lvl1",
      "charge2": "bullet_charge_lvl2",
      "charge3": "bullet_charge_lvl3",
      "charge4": "bullet_charge_lvl4",
      "charge5": "bullet_charge_lvl5"
    }
  }
}
```

- `animations`: list of clips with arbitrary frames; frames can be uneven per row/column.
- `labels`: per `spriteId`, map a friendly name (row1/player1/etc.) to a clip id.

## Resolution rules

For each archetype (from `LevelInit`):
- `spriteId` is required.
- `animId` (optional) is resolved as:
  1) exact clip id in registry,
  2) `labels[spriteId][animId]` if present,
  3) fallback to clip whose id == `spriteId`.
- If `animId` is provided but no clip/label matches, the archetype is skipped and an error is logged.

The chosen clip sets:
- Custom frame rects on `SpriteComponent`.
- `AnimationComponent` frame count/order/time, loop flag.

## Server contract

- Server never handles assets; it just sends `spriteId` and optionally `animId` in `LevelInit`.
- Example: to pick a different row of the player sheet, send `spriteId="player_ship"` and `animId="player3"` (mapped via labels to the right row).
- For bullets, the base animation id is `bullet_basic`; charge levels are `bullet_charge_lvl1`..`bullet_charge_lvl5` (or use the `charge`/`chargeN` labels for the sprite `bullet`).
