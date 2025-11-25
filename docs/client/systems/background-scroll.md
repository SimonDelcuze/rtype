# BackgroundScrollSystem

This system handles infinite horizontal scrolling backgrounds with optional parallax layers. It keeps a strip of sprites moving, clones new strips to fill the window, and wraps strips that exit the left edge. It can also swap textures on the fly for the next wrap.

## Data
- `TransformComponent` (required): `x`, `y`, `rotation`, `scaleX`, `scaleY`.
- `BackgroundScrollComponent` (required): `speedX`, `speedY`, `resetOffsetX`, `resetOffsetY`. Offsets are the wrapped width/height in world units. If zero, they are inferred from the texture size.
- `SpriteComponent` (required): must hold a valid sprite/texture.
- `LayerComponent` (optional): z-order layer copied to clones.

## Algorithm (per `update`)
1) Collect entries: gather entities with Transform + BackgroundScroll + Sprite; skip missing/empty sprites; capture texture + layer (if any).
2) Scale and offsets: scale sprites to fit window height; set/reset `resetOffsetX/Y` to texture size * scale; cache width/height.
3) Move: advance positions by `speedX/speedY * deltaTime`, track min/max X.
4) Coverage: while total coverage `(maxX + width) - minX` is less than window width + one strip, spawn clones to the right (copy sprite/scroll/layer, same scale).
5) Wrap/swap: find current maxX; any strip whose `x <= -resetOffsetX` is moved to `maxX + resetOffsetX`; if `nextTexture_` is set, the wrap applies the new texture and recomputes scale/offsets. Y wraps similarly if `resetOffsetY > 0` and strip leaves the top.
6) Clear `nextTexture_` after applying pending swap.

## Texture swap
Call `setNextBackground(texture)`; the next wrapped strip applies this texture and recomputes scale/offsets.

## Notes
- Assumes horizontal scrolling to the left (negative `speedX`) for wrapping; positive speeds still advance but wrapping triggers when `x <= -resetOffsetX`.
- `resetOffsetX/Y` defaults to texture size * scale when 0. Use explicit values for custom tiling.
- Coverage clones only use the first entry as a template; ensure all background strips share speed/scale to avoid gaps.
