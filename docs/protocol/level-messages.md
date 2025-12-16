# RType Level and Snapshot Extensions (RFC-style)

## Status of This Memo
This document specifies the level-setup and snapshot-related messages for the RType UDP protocol. Normative language follows [RFC 2119].

## 1. Scope
This memo complements the core specification in `docs/global-overview/network_protocol.md` and details level initialization, snapshot semantics, and snapshot chunking. All messages inherit the common header and CRC rules from the core spec.

## 2. Message Types (Server → Client)
- `0x30 LevelInit` — Level metadata and archetypes (Section 3).
- `0x31 LevelTransition` — Reserved; not emitted in the current build (Section 4).
- `0x14 Snapshot` — State updates with typed entities (Section 5).
- `0x21 SnapshotChunk` — Chunked snapshot delivery (Section 6).

## 3. LevelInit (0x30)
Announces the upcoming level and asset identifiers.
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
Clients SHOULD map ids to a local manifest and MAY use placeholders when unknown. The `seed` SHOULD be stored for deterministic spawns/pathing.

## 4. LevelTransition (0x31)
Reserved for future use and not emitted in the current build. Clients MAY ignore this message if received.

## 5. Snapshot (0x14)
Snapshots use an update mask that reflects the presence of components (not deltas).
### 5.1 Per-entity layout
```
u32 entityId
u16 updateMask
[fields gated by mask bits]
```
### 5.2 Mask bits
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

### 5.3 Packet payload
```
u16 entityCount
[entity blocks as above]
```
Clients MUST validate payload size and CRC before parsing. Unknown bits SHOULD be ignored; unknown entityType values SHOULD be logged and handled with placeholders.

## 6. SnapshotChunk (0x21)
Used when a snapshot is split because of payload limits.
```
u16 totalChunks
u16 chunkIndex   // 0-based
u16 entityCountInChunk
[chunked entity blocks with the same layout as Section 5]
```
Receivers SHOULD reassemble chunks by `chunkIndex` and discard sets where `totalChunks` disagrees.

## 7. CRC Requirement
All messages described here MUST append a CRC32 footer (big-endian) computed over `[Header + Payload]`. The reference implementation is `PacketHeader::crc32`.

## 8. Reliability Considerations
- `LevelInit` SHOULD be retried until acknowledged by the client to avoid stalls due to UDP loss.
- `Snapshot` and `SnapshotChunk` are sent best-effort; dropped packets may be recovered by later snapshots.

## 9. References
[RFC 2119] Bradner, S., "Key words for use in RFCs to Indicate Requirement Levels", BCP 14, March 1997.
