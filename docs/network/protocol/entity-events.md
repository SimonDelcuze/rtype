#Entity / Event Packets

This section documents the small event -
        style packets sent by the server to keep clients in sync when players disconnect or
    entities appear / disappear.All packets use the shared 15 - byte header,
    big - endian encoding,
    and a 4 -
            byte CRC32 footer like the other binary messages.

            ##PlayerDisconnectedPacket(`MessageType = 0x1C`) -
            Direction : Server → Client - Payload size : 4 bytes -
                        Payload fields(big - endian)
    : - `playerId` (`uint32`)
    : Identifier of the player that left -
      CRC32 over `[header + payload]`

      Use : broadcast to every connected client when a player times out
        or disconnects.

               ##EntitySpawnPacket(`MessageType = 0x1D`) -
               Direction : Server → Client - Payload size : 13 bytes -
                           Payload fields(big - endian)
    : - `entityId` (`uint32`) - `entityType` (`uint8`) — game -
      defined type / class - `posX` (`float32`) - `posY` (`float32`) -
      CRC32 over `[header + payload]`

      Use : notify clients of a new authoritative entity.Positions must be finite; packet is dropped otherwise.

## EntityDestroyedPacket (`MessageType = 0x1E`)
- Direction: Server → Client
- Payload size: 4 bytes
- Payload fields (big-endian):
  - `entityId` (`uint32`)
- CRC32 over `[header + payload]`

Use: announce authoritative removal of an entity. Clients should purge local state for the given `entityId`.

## Notes
- All three packets set `packetType = ServerToClient` and `version = PacketHeader::kProtocolVersion`.
- Receivers must validate magic, version, messageType, payloadSize, and CRC before acting.
- `sequenceId`/`tickId` are filled by the server; clients can use them for ordering if needed.
