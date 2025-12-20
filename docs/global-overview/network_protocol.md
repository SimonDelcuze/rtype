# RType UDP Protocol Specification (RFC-style)

## Status of This Memo
This document describes the RType binary UDP protocol used between clients and the game server. It is intended as a stable reference for implementers. The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY**, and **OPTIONAL** in this document are to be interpreted as described in [RFC 2119].

## 1. Introduction
The RType UDP protocol is a compact, binary protocol optimized for real-time gameplay. It carries authoritative state from the server to clients and player inputs from clients to the server. All traffic uses UDP; reliability is provided only where explicitly stated.

## 2. Packet Format Overview
- All fields are big-endian unless noted.
- Every packet on the wire has the following layout:
  ```
  [Header][Payload][CRC32]
  ```
- The CRC32 footer is **mandatory** on every packet (Section 4).

## 3. Conventions and Terminology
- **Client**: the game client sending inputs and consuming state.
- **Server**: the authoritative simulation producing snapshots.
- **Payload Length**: byte length of the payload portion only.
- **Sequence ID**: monotonic value used for ordering; wrapping is allowed.
- **Tick ID**: server simulation tick (when applicable).

## 4. Packet Header
The header is exactly 15 bytes and MUST precede every payload.
```
0-3   Magic        0xA3 0x5F 0xC8 0x1D
4     Version      = 0x01
5     Packet Type  0x01 ClientToServer | 0x02 ServerToClient
6     Message Type (see Section 5)
7-8   Sequence ID  u16
9-12  Tick ID      u32
13-14 Payload Len  u16
```
Implementations MUST reject packets whose magic or version do not match the values above.

## 5. Message Types
Unless otherwise stated, payloads MAY be zero-length. Message types are scoped by Packet Type but enumerated here for convenience.

### 5.1 Client to Server
- 0x01 CLIENT_HELLO — no payload
- 0x02 CLIENT_JOIN_REQUEST — no payload
- 0x03 CLIENT_READY — no payload
- 0x04 CLIENT_PING — `u32 clientTime`
- 0x05 CLIENT_INPUT — see Section 6.2
- 0x06 CLIENT_ACKNOWLEDGE — `u16 sequenceId, u8 messageType`
- 0x07 CLIENT_DISCONNECT — `u8 reasonCode`

### 5.2 Server to Client
- 0x10 SERVER_HELLO — no payload
- 0x11 SERVER_JOIN_ACCEPT — no payload
- 0x12 SERVER_JOIN_DENY — no payload
- 0x13 SERVER_PONG — `u32 clientTime` (echo)
- 0x14 SERVER_SNAPSHOT — see Section 6.1
- 0x15 SERVER_GAME_START — no payload
- 0x16 SERVER_GAME_END — `u32 winningPlayerId`
- 0x17 SERVER_KICK — `u8 reasonCode`
- 0x18 SERVER_BAN — `u8 reasonCode`
- 0x19 SERVER_BROADCAST — `u8 len, bytes[len]`
- 0x1A SERVER_DISCONNECT — `u8 reasonCode`
- 0x1B SERVER_ACKNOWLEDGE — `u16 sequenceId, u8 messageType`
- 0x1C SERVER_PLAYER_DISCONNECTED — `u32 playerId`
- 0x1D SERVER_ENTITY_SPAWN — `u32 entityId, u8 entityType, f32 posX, f32 posY`
- 0x1E SERVER_ENTITY_DESTROYED — `u32 entityId`
- 0x1F SERVER_ALL_READY — no payload
- 0x20 SERVER_COUNTDOWN_TICK — `u8 value`
- 0x21 SERVER_SNAPSHOT_CHUNK — see Section 6.3
- 0x30 SERVER_LEVEL_INIT — see Section 7.1
- 0x31 SERVER_LEVEL_TRANSITION — reserved for future use (not emitted yet)

## 6. State Synchronization

### 6.1 SERVER_SNAPSHOT (0x14)
Sent at a regular cadence (e.g., 60 Hz). The snapshot mask reflects component presence (not deltas).
- Entity Count: `u16`
- For each entity:
  - `u32 entityId`
  - `u16 updateMask`
  - Conditional fields gated by mask bits:
    - bit0: `u8 entityType` (currently always set)
    - bit1: `f32 posX`
    - bit2: `f32 posY`
    - bit3: `f32 velX` if VelocityComponent exists
    - bit4: `f32 velY` if VelocityComponent exists
    - bit5: `u16 health` if HealthComponent exists
    - bit6: `u8 statusFlags` when InvincibilityComponent is present (uses bit1)
    - bit7: reserved
    - bit8: reserved
    - bit9: `u8 lives` if LivesComponent exists
Clients MUST validate `payloadSize` and CRC before parsing and SHOULD ignore unknown bits.

### 6.2 CLIENT_INPUT (0x05)
Payload (fixed 18 bytes):
```
u32 playerId
u16 inputFlags
f32 posX
f32 posY
f32 angle
```
`inputFlags` are bitwise values representing movement and actions; non-finite floats MUST be rejected.

### 6.3 SERVER_SNAPSHOT_CHUNK (0x21)
Used when a snapshot is split into multiple packets.
```
u16 totalChunks
u16 chunkIndex
u16 entityCountInChunk
[chunked entity blocks with the same layout as Section 6.1]
```
Chunks are indexed from 0. Receivers SHOULD ignore chunks whose `totalChunks` disagree across fragments.

## 7. Level Flow

### 7.1 SERVER_LEVEL_INIT (0x30)
Announces content ids and archetypes for the upcoming level.
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
Unknown ids SHOULD be logged and substituted with placeholders.

### 7.2 SERVER_LEVEL_TRANSITION (0x31)
Reserved for future use; not emitted in the current build. Clients MAY ignore it if received.

### 7.3 SERVER_LEVEL_EVENT (0x32)
Runtime level events for scroll, background, music, camera bounds, and gates.
```
u8 eventType
switch eventType:
  1 set_scroll:
     u8  mode
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

## 8. Integrity Protection
All packets MUST append a 4-byte CRC32 footer in big-endian order. The CRC is computed over `[Header + Payload]`. Receivers MUST discard packets whose CRC verification fails.

## 9. Reason Codes
Reason codes are 1 byte. The following values are reserved:
- 0x00 Unknown
- 0x01 Requested by Client
- 0x10 Inactivity
- 0x11 Protocol Violation
- 0x12 Overflow / Spam
- 0x13 Unauthorized Action
- 0x20 Cheating Detected
- 0x21 Manual Ban
- 0x30 Server Shutdown
- 0x31 Server Restart
Unknown codes SHOULD be treated as non-fatal and displayed generically.

## 10. Security Considerations
Implementations MUST validate magic, version, `payloadSize`, and CRC before acting on data. Clients SHOULD rate-limit or drop malformed packets to prevent abuse.

## 11. IANA Considerations
This protocol uses a private UDP port negotiated externally. No IANA action is required.

## 12. References
[RFC 2119] Bradner, S., "Key words for use in RFCs to Indicate Requirement Levels", BCP 14, March 1997.
