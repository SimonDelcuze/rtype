# RType Level and Snapshot Extensions (RFC-style)

## Status of This Memo
This document specifies the level-setup and snapshot-related messages for the RType UDP protocol. Normative language follows [RFC 2119].

## 1. Scope
This memo complements the core specification in `docs/global-overview/network_protocol.md` and details level initialization, runtime events, snapshot semantics, and snapshot chunking. All messages inherit the common header and CRC rules from the core spec.

`LevelInit` is derived from the JSON level meta and archetypes. `LevelEvent` is emitted by the server runtime (`LevelDirector`) as the timeline advances.

## 2. Message Types (Server → Client)
- `0x30 LevelInit` — Level metadata and archetypes (Section 3).
- `0x31 LevelTransition` — Reserved; not emitted in the current build (Section 5).
- `0x32 LevelEvent` — Runtime level events (Section 4).
- `0x14 Snapshot` — State updates with typed entities (Section 6).
- `0x21 SnapshotChunk` — Chunked snapshot delivery (Section 7).

## 3. LevelInit (0x30)
Announces the upcoming level and asset identifiers. The `backgroundId` and `musicId` originate from the level meta.
Background is applied immediately by the client; music playback is typically triggered by a `LevelEvent set_music`.
```
u16 levelId
u32 seed
u8  backgroundIdLen, backgroundId[Len]
u8  musicIdLen, musicId[Len]
u8  archetypeCount
repeat archetypeCount times:
    u16 typeId
    u8  spriteIdLen, spriteId[Len]
    u8  animIdLen, animId[Len]   // 0 length = no animation
    u8  layer
```
Clients SHOULD map ids to a local manifest and MAY use placeholders when unknown. The `seed` SHOULD be stored for deterministic spawns/pathing. Empty string ids SHOULD be treated as "no asset".

## 4. LevelEvent (0x32)
Runtime events for scroll, background, music, camera bounds, and gates.
`tickId` in the packet header may be used to order events, but clients MUST still handle out-of-order delivery safely.
```
u8 eventType
switch eventType:
  1 set_scroll:
     u8  mode           // 0=constant, 1=stopped, 2=curve
     f32 speedX
     u8  keyframeCount
     repeat keyframeCount times:
         f32 time
         f32 speedX
  2 set_background:
     u8  backgroundIdLen, backgroundId[Len]
  3 set_music:
     u8  musicIdLen, musicId[Len]
  4 set_camera_bounds:
     f32 minX
     f32 maxX
     f32 minY
     f32 maxY
  5 gate_open:
     u8  gateIdLen, gateId[Len]
  6 gate_close:
     u8  gateIdLen, gateId[Len]
```
For `curve`, keyframes are applied as step changes at `time` (relative to event receipt). Keyframes SHOULD be sorted by time. If `keyframeCount` is zero, clients SHOULD treat the curve as stopped.

Clients MUST ignore unknown eventType values.

## 5. LevelTransition (0x31)
Reserved for future use and not emitted in the current build. Clients MAY ignore this message if received.

## 6. Snapshot (0x14)
Snapshots use an update mask that reflects the presence of components (not deltas).
### 6.1 Per-entity layout
```
u32 entityId
u16 updateMask
[fields gated by mask bits]
```
### 6.2 Mask bits
- bit0: `u8 entityType` — currently always set
- bit1: `f32 posX`
- bit2: `f32 posY`
- bit3: `f32 velX` if VelocityComponent exists
- bit4: `f32 velY` if VelocityComponent exists
- bit5: `u16 health` if HealthComponent exists
- bit6: `u8 statusFlags` when InvincibilityComponent is present (uses bit1)
- bit7: reserved
- bit8: reserved
- bit9: `u8 lives` if LivesComponent exists

### 6.3 Packet payload
```
u16 entityCount
[entity blocks as above]
```
Clients MUST validate payload size and CRC before parsing. Unknown bits SHOULD be ignored; unknown entityType values SHOULD be logged and handled with placeholders.

## 7. SnapshotChunk (0x21)
Used when a snapshot is split because of payload limits.
```
u16 totalChunks
u16 chunkIndex   // 0-based
u16 entityCountInChunk
[chunked entity blocks with the same layout as Section 6]
```
Receivers SHOULD reassemble chunks by `chunkIndex` and discard sets where `totalChunks` disagrees.

## 8. CRC Requirement
All messages described here MUST append a CRC32 footer (big-endian) computed over `[Header + Payload]`. The reference implementation is `PacketHeader::crc32`.

## 9. Reliability Considerations
- `LevelInit` SHOULD be retried until acknowledged by the client to avoid stalls due to UDP loss.
- `LevelEvent` is best-effort; clients SHOULD tolerate missed events (later events override earlier state).
- `Snapshot` and `SnapshotChunk` are sent best-effort; dropped packets may be recovered by later snapshots.

## 10. References
[RFC 2119] Bradner, S., "Key words for use in RFCs to Indicate Requirement Levels", BCP 14, March 1997.
