# Level Protocol and Entity Typing

Defines the messages the client consumes for level setup, transitions, and typed entity replication. Payloads are appended after the common `PacketHeader` (15 bytes). Unless stated otherwise, strings are ASCII, length‑prefixed with a single byte (max 255).

## Message types
- `LevelInit` (`messageType = 0x30`): announces a level and the assets/archetypes to preload.
- `LevelTransition` (`messageType = 0x31`): signals the end of a level and the next level metadata.
- `Snapshot` (`messageType = 0x14`): real‑time state updates; now carries `entityType` when requested by the mask.

## LevelInit payload
```
u16 levelId
u32 seed
u8  backgroundIdLen, backgroundId[Len]
u8  musicIdLen, musicId[Len]
u8  archetypeCount
repeat archetypeCount times:
    u16 typeId
    u8  spriteIdLen, spriteId[Len]
    u8  animIdLen, animId[Len]   // 0 length means “no animation”
    u8  layer                    // render layer, unsigned
```
- The client maps `spriteId` / `animId` / `backgroundId` / `musicId` to its local manifest; unknown ids should log a warning and use placeholders.
- `seed` is stored for deterministic spawns/pathing.

## LevelTransition payload
```
u16 nextLevelId
u32 nextSeed
u8  reason   // 0 = completed, 1 = failed, 2 = skipped, 3 = server-initiated
```
- Client cleans up current level entities and preloads assets for `nextLevelId` if already known.

## Level coordination flow
1. `LevelInit` is sent once per level before gameplay snapshots. Client validates protocolVersion, caches archetypes, and preloads assets.
2. Snapshots stream during the level and include `entityType` for any entity whose type may be unknown to the client.
3. When a level ends, `LevelTransition` signals the next level and seed; client tears down current entities and can start preloading next assets immediately.
4. If a `LevelTransition` arrives without a known manifest entry for the announced ids, client should warn, keep running, and use placeholders.

### Client-facing rules for multi-level stability
- LevelInit / LevelTransition should be retried until the client acknowledges (tiny ACK) to avoid being stuck on packet loss.
- The first snapshot referencing an unknown entityId must carry `entityType`; after a transition, assume all entities are unknown until typed snapshots arrive.
- Drop snapshots whose levelId does not match the active level to avoid cross-level ghosts.
- On transition, destroy entities from the previous level and reset interpolation/prediction state before applying new snapshots.
- Gameplay resumes after assets from LevelInit are preloaded or placeholders are installed; showing a loading/transition screen is acceptable.

## Snapshot payload (typed entities)
Snapshots use an update mask to include only present fields. Mask bits:
```
bit0: entityType      (u16 typeId)
bit1: posX            (f32)
bit2: posY            (f32)
bit3: velX            (f32)
bit4: velY            (f32)
bit5: health          (u16)
bit6: statusEffects   (u8)   // optional flags
bit7: orientation     (f32)
bit8: dead            (u8, 0/1)
```
Payload layout per entity:
```
u32 entityId
u16 updateMask
[fields gated by mask bits above]
```
- When `entityType` is present, the client creates/updates a local entity using the archetype from `LevelInit`.
- If a snapshot arrives without `entityType` for an unknown `entityId`, the client should warn and skip creation.
- When `dead` is set, the client destroys the local entity.

## CRC
If CRC is used, append `CRC32` (big‑endian u32) after the payload. `PacketHeader::crc32` is the reference implementation. The client should validate when present and ignore packets that fail.
