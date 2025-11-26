# Snapshot Parser (Client)

Parses `SERVER_SNAPSHOT` packets into structured data (`SnapshotParseResult`) that can be applied by the replication system.

## Input
- Raw `std::vector<uint8_t>` buffers dequeued from the network message handler.
- Expected layout: `[Header][Payload][CRC32]` with `PacketType::ServerToClient` and `MessageType::Snapshot`.

## Validation
- Header: magic, protocol version, packet type = ServerToClient, message type = Snapshot.
- Sizes: payload size matches buffer length (including CRC).
- CRC: recomputed over header + payload and compared to the transmitted CRC32.
- If any check fails, parsing returns `nullopt`.

## Payload layout
- `entityCount` (2 bytes)
- For each entity:
  - `entityId` (4 bytes)
  - `updateMask` (2 bytes)
  - Optional fields if bits are set:
    - bit 0: `entityType` (1 byte)
    - bit 1: `posX` (float)
    - bit 2: `posY` (float)
    - bit 3: `velX` (float)
    - bit 4: `velY` (float)
    - bit 5: `health` (int16)
    - bit 6: `statusEffects` (1 byte)
    - bit 7: `orientation` (float)
    - bit 8: `dead` (1 byte, 0 = alive, 1 = dead)

## Output
- `SnapshotParseResult` containing:
  - `PacketHeader header`
  - `std::vector<SnapshotEntity> entities`
    - Each `SnapshotEntity` holds `entityId`, `updateMask`, and optional values for the fields indicated by the mask.

## Usage
- Invoked by `NetworkMessageHandler` when a snapshot packet is dequeued.
- The parsed result is pushed to a thread-safe queue for the replication system.

## Tests
- `tests/client/network/SnapshotParserTests.cpp` covers full parse, multiple entities, missing data, wrong packet/message type, CRC mismatch, and size validation.
