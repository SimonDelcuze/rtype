# Lobby Protocol

The **Lobby Protocol** is a binary UDP protocol extension that enables clients to discover, create, and join game instances in the multi-instance server architecture.

This protocol operates **separately** from the game protocol.\
Clients connect to the lobby server first, then transition to a game instance using the standard game protocol.

***

## **1. Overview**

The lobby protocol consists of **7 message types**:

### Client → Lobby

* `LOBBY_LIST_ROOMS` (0x40) - Request list of available rooms
* `LOBBY_CREATE_ROOM` (0x42) - Request to create new room
* `LOBBY_JOIN_ROOM` (0x44) - Request to join existing room

### Lobby → Client

* `LOBBY_ROOM_LIST` (0x41) - Response with room listings
* `LOBBY_ROOM_CREATED` (0x43) - Confirmation of room creation
* `LOBBY_JOIN_SUCCESS` (0x45) - Join approved with port info
* `LOBBY_JOIN_FAILED` (0x46) - Join rejected

All packets use the standard `PacketHeader` structure from the existing protocol.

***

## **2. Message Type Enumeration**

```cpp
enum class MessageType : std::uint8_t
{
    // ... existing game protocol messages ...

    // Lobby Protocol (0x40-0x46)
    LobbyListRooms   = 0x40,  // Client → Lobby: Request room list
    LobbyRoomList    = 0x41,  // Lobby → Client: Room list response
    LobbyCreateRoom  = 0x42,  // Client → Lobby: Create new room
    LobbyRoomCreated = 0x43,  // Lobby → Client: Room creation success
    LobbyJoinRoom    = 0x44,  // Client → Lobby: Join existing room
    LobbyJoinSuccess = 0x45,  // Lobby → Client: Join approved
    LobbyJoinFailed  = 0x46,  // Lobby → Client: Join rejected
};
```

***

## **3. Packet Structure**

All lobby packets follow this structure:

```
┌────────────────────────────────────┐
│     PacketHeader (varies)          │  Variable size (see below)
├────────────────────────────────────┤
│     Payload (varies)               │  Variable size (message-specific)
├────────────────────────────────────┤
│     CRC32 Checksum (4 bytes)       │  Integrity verification
└────────────────────────────────────┘
```

### PacketHeader Structure

```cpp
struct PacketHeader
{
    std::uint8_t  packetType;    // ClientToServer (0x01) or ServerToClient (0x02)
    std::uint8_t  messageType;   // One of the MessageType enum values
    std::uint16_t sequenceId;    // Incrementing sequence number
    std::uint16_t payloadSize;   // Size of payload section
    std::uint16_t originalSize;  // Size before compression (same if not compressed)

    static constexpr std::size_t kSize = 8;       // Header size
    static constexpr std::size_t kCrcSize = 4;    // CRC32 size
};
```

### CRC32 Calculation

```cpp
std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
// Append as big-endian 4 bytes
```

***

## **4. LOBBY_LIST_ROOMS**

### Direction

Client → Lobby

### Purpose

Request list of all available game rooms.

### Packet Format

```
┌──────────────────────────┐
│ PacketHeader             │
│ - packetType: 0x01       │
│ - messageType: 0x40      │
│ - sequenceId: N          │
│ - payloadSize: 0         │
│ - originalSize: 0        │
├──────────────────────────┤
│ CRC32 (4 bytes)          │
└──────────────────────────┘

Total Size: 8 + 4 = 12 bytes
```

### Payload

None (zero-length payload).

### Example Build

```cpp
std::vector<std::uint8_t> buildListRoomsPacket(std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType   = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType  = static_cast<std::uint8_t>(MessageType::LobbyListRooms);
    hdr.sequenceId   = sequence;
    hdr.payloadSize  = 0;
    hdr.originalSize = 0;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}
```

***

## **5. LOBBY_ROOM_LIST**

### Direction

Lobby → Client

### Purpose

Response containing list of available rooms.

### Packet Format

```
┌──────────────────────────────────────┐
│ PacketHeader                         │
│ - packetType: 0x02                   │
│ - messageType: 0x41                  │
│ - sequenceId: N                      │
│ - payloadSize: 2 + (roomCount × 13) │
│ - originalSize: same                 │
├──────────────────────────────────────┤
│ uint16_t: roomCount                  │  Big-endian
├──────────────────────────────────────┤
│ For each room (13 bytes):            │
│   uint32_t: roomId                   │  Big-endian
│   uint16_t: playerCount              │  Big-endian
│   uint16_t: maxPlayers               │  Big-endian
│   uint16_t: port                     │  Big-endian
│   uint8_t:  state                    │  Room state enum
├──────────────────────────────────────┤
│ CRC32 (4 bytes)                      │
└──────────────────────────────────────┘
```

### Room State Enum

```cpp
enum class RoomState : std::uint8_t
{
    Waiting   = 0,  // Room created, waiting for players
    Countdown = 1,  // Countdown before game starts
    Playing   = 2,  // Game in progress
    Finished  = 3   // Game completed
};
```

### RoomInfo Structure

```cpp
struct RoomInfo
{
    std::uint32_t roomId;        // Unique room identifier
    std::size_t   playerCount;   // Current player count
    std::size_t   maxPlayers;    // Maximum players allowed
    RoomState     state;         // Current room state
    std::uint16_t port;          // UDP port for this instance
};
```

### Payload Size Calculation

```cpp
std::uint16_t payloadSize = sizeof(uint16_t) +  // roomCount field
                            (roomCount * 13);    // 13 bytes per room
```

### Example Build

```cpp
std::vector<std::uint8_t> buildRoomListPacket(const std::vector<RoomInfo>& rooms,
                                              std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyRoomList);
    hdr.sequenceId  = sequence;

    std::uint16_t roomCount = static_cast<std::uint16_t>(rooms.size());
    std::uint16_t payloadSize = sizeof(std::uint16_t) + (roomCount * 13);

    hdr.payloadSize  = payloadSize;
    hdr.originalSize = payloadSize;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize);

    // Encode header
    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    // Encode room count
    packet.push_back(static_cast<std::uint8_t>((roomCount >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomCount & 0xFF));

    // Encode each room
    for (const auto& room : rooms) {
        // roomId (4 bytes, big-endian)
        packet.push_back(static_cast<std::uint8_t>((room.roomId >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((room.roomId >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((room.roomId >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(room.roomId & 0xFF));

        // playerCount (2 bytes, big-endian)
        std::uint16_t playerCount = static_cast<std::uint16_t>(room.playerCount);
        packet.push_back(static_cast<std::uint8_t>((playerCount >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(playerCount & 0xFF));

        // maxPlayers (2 bytes, big-endian)
        std::uint16_t maxPlayers = static_cast<std::uint16_t>(room.maxPlayers);
        packet.push_back(static_cast<std::uint8_t>((maxPlayers >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(maxPlayers & 0xFF));

        // port (2 bytes, big-endian)
        packet.push_back(static_cast<std::uint8_t>((room.port >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(room.port & 0xFF));

        // state (1 byte)
        packet.push_back(static_cast<std::uint8_t>(room.state));
    }

    // Append CRC32
    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}
```

### Example Parse

```cpp
std::optional<RoomListResult> parseRoomListPacket(const std::uint8_t* data,
                                                   std::size_t size)
{
    if (size < PacketHeader::kSize + sizeof(std::uint16_t)) {
        return std::nullopt;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;
    std::uint16_t roomCount = (static_cast<std::uint16_t>(payload[0]) << 8) |
                              static_cast<std::uint16_t>(payload[1]);

    constexpr std::size_t roomInfoSize = 13;

    if (size < PacketHeader::kSize + sizeof(std::uint16_t) +
               (roomCount * roomInfoSize) + PacketHeader::kCrcSize) {
        return std::nullopt;
    }

    RoomListResult result;
    result.rooms.reserve(roomCount);

    const std::uint8_t* ptr = payload + sizeof(std::uint16_t);

    for (std::uint16_t i = 0; i < roomCount; ++i) {
        RoomInfo info;

        info.roomId = (static_cast<std::uint32_t>(ptr[0]) << 24) |
                      (static_cast<std::uint32_t>(ptr[1]) << 16) |
                      (static_cast<std::uint32_t>(ptr[2]) << 8) |
                      static_cast<std::uint32_t>(ptr[3]);
        ptr += 4;

        info.playerCount = (static_cast<std::uint16_t>(ptr[0]) << 8) |
                           static_cast<std::uint16_t>(ptr[1]);
        ptr += 2;

        info.maxPlayers = (static_cast<std::uint16_t>(ptr[0]) << 8) |
                          static_cast<std::uint16_t>(ptr[1]);
        ptr += 2;

        info.port = (static_cast<std::uint16_t>(ptr[0]) << 8) |
                    static_cast<std::uint16_t>(ptr[1]);
        ptr += 2;

        info.state = static_cast<RoomState>(ptr[0]);
        ptr += 1;

        result.rooms.push_back(info);
    }

    return result;
}
```

***

## **6. LOBBY_CREATE_ROOM**

### Direction

Client → Lobby

### Purpose

Request creation of a new game instance.

### Packet Format

```
┌──────────────────────────┐
│ PacketHeader             │
│ - packetType: 0x01       │
│ - messageType: 0x42      │
│ - sequenceId: N          │
│ - payloadSize: 0         │
│ - originalSize: 0        │
├──────────────────────────┤
│ CRC32 (4 bytes)          │
└──────────────────────────┘

Total Size: 8 + 4 = 12 bytes
```

### Payload

None (zero-length payload).

### Example Build

```cpp
std::vector<std::uint8_t> buildCreateRoomPacket(std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType   = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType  = static_cast<std::uint8_t>(MessageType::LobbyCreateRoom);
    hdr.sequenceId   = sequence;
    hdr.payloadSize  = 0;
    hdr.originalSize = 0;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}
```

***

## **7. LOBBY_ROOM_CREATED**

### Direction

Lobby → Client

### Purpose

Confirmation that a new room was created successfully.

### Packet Format

```
┌──────────────────────────┐
│ PacketHeader             │
│ - packetType: 0x02       │
│ - messageType: 0x43      │
│ - sequenceId: N          │
│ - payloadSize: 6         │
│ - originalSize: 6        │
├──────────────────────────┤
│ uint32_t: roomId         │  Big-endian
│ uint16_t: port           │  Big-endian
├──────────────────────────┤
│ CRC32 (4 bytes)          │
└──────────────────────────┘

Total Size: 8 + 6 + 4 = 18 bytes
```

### Payload

* **roomId** (4 bytes): Unique identifier for the created room
* **port** (2 bytes): UDP port where the game instance is listening

### Example Build

```cpp
std::vector<std::uint8_t> buildRoomCreatedPacket(std::uint32_t roomId,
                                                 std::uint16_t port,
                                                 std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyRoomCreated);
    hdr.sequenceId  = sequence;

    constexpr std::uint16_t payloadSize = 6;  // 4 + 2
    hdr.payloadSize  = payloadSize;
    hdr.originalSize = payloadSize;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    // roomId (4 bytes, big-endian)
    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    // port (2 bytes, big-endian)
    packet.push_back(static_cast<std::uint8_t>((port >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(port & 0xFF));

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}
```

### Example Parse

```cpp
std::optional<RoomCreatedResult> parseRoomCreatedPacket(const std::uint8_t* data,
                                                         std::size_t size)
{
    constexpr std::size_t expectedSize = PacketHeader::kSize + 6 + PacketHeader::kCrcSize;

    if (size < expectedSize) {
        return std::nullopt;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;

    RoomCreatedResult result;
    result.roomId = (static_cast<std::uint32_t>(payload[0]) << 24) |
                    (static_cast<std::uint32_t>(payload[1]) << 16) |
                    (static_cast<std::uint32_t>(payload[2]) << 8) |
                    static_cast<std::uint32_t>(payload[3]);

    result.port = (static_cast<std::uint16_t>(payload[4]) << 8) |
                  static_cast<std::uint16_t>(payload[5]);

    return result;
}
```

***

## **8. LOBBY_JOIN_ROOM**

### Direction

Client → Lobby

### Purpose

Request to join an existing game room.

### Packet Format

```
┌──────────────────────────┐
│ PacketHeader             │
│ - packetType: 0x01       │
│ - messageType: 0x44      │
│ - sequenceId: N          │
│ - payloadSize: 4         │
│ - originalSize: 4        │
├──────────────────────────┤
│ uint32_t: roomId         │  Big-endian
├──────────────────────────┤
│ CRC32 (4 bytes)          │
└──────────────────────────┘

Total Size: 8 + 4 + 4 = 16 bytes
```

### Payload

* **roomId** (4 bytes): ID of the room to join

### Example Build

```cpp
std::vector<std::uint8_t> buildJoinRoomPacket(std::uint32_t roomId,
                                              std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyJoinRoom);
    hdr.sequenceId  = sequence;

    constexpr std::uint16_t payloadSize = 4;
    hdr.payloadSize  = payloadSize;
    hdr.originalSize = payloadSize;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    // roomId (4 bytes, big-endian)
    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}
```

***

## **9. LOBBY_JOIN_SUCCESS**

### Direction

Lobby → Client

### Purpose

Confirmation that join request was approved.

### Packet Format

```
┌──────────────────────────┐
│ PacketHeader             │
│ - packetType: 0x02       │
│ - messageType: 0x45      │
│ - sequenceId: N          │
│ - payloadSize: 6         │
│ - originalSize: 6        │
├──────────────────────────┤
│ uint32_t: roomId         │  Big-endian
│ uint16_t: port           │  Big-endian
├──────────────────────────┤
│ CRC32 (4 bytes)          │
└──────────────────────────┘

Total Size: 8 + 6 + 4 = 18 bytes
```

### Payload

* **roomId** (4 bytes): Confirmed room ID
* **port** (2 bytes): UDP port where the game instance is listening

### Example Build

```cpp
std::vector<std::uint8_t> buildJoinSuccessPacket(std::uint32_t roomId,
                                                 std::uint16_t port,
                                                 std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyJoinSuccess);
    hdr.sequenceId  = sequence;

    constexpr std::uint16_t payloadSize = 6;
    hdr.payloadSize  = payloadSize;
    hdr.originalSize = payloadSize;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    // roomId (4 bytes, big-endian)
    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    // port (2 bytes, big-endian)
    packet.push_back(static_cast<std::uint8_t>((port >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(port & 0xFF));

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}
```

### Example Parse

```cpp
std::optional<JoinSuccessResult> parseJoinSuccessPacket(const std::uint8_t* data,
                                                         std::size_t size)
{
    constexpr std::size_t expectedSize = PacketHeader::kSize + 6 + PacketHeader::kCrcSize;

    if (size < expectedSize) {
        return std::nullopt;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;

    JoinSuccessResult result;
    result.roomId = (static_cast<std::uint32_t>(payload[0]) << 24) |
                    (static_cast<std::uint32_t>(payload[1]) << 16) |
                    (static_cast<std::uint32_t>(payload[2]) << 8) |
                    static_cast<std::uint32_t>(payload[3]);

    result.port = (static_cast<std::uint16_t>(payload[4]) << 8) |
                  static_cast<std::uint16_t>(payload[5]);

    return result;
}
```

***

## **10. LOBBY_JOIN_FAILED**

### Direction

Lobby → Client

### Purpose

Indicates that join request was rejected.

### Packet Format

```
┌──────────────────────────┐
│ PacketHeader             │
│ - packetType: 0x02       │
│ - messageType: 0x46      │
│ - sequenceId: N          │
│ - payloadSize: 0         │
│ - originalSize: 0        │
├──────────────────────────┤
│ CRC32 (4 bytes)          │
└──────────────────────────┘

Total Size: 8 + 4 = 12 bytes
```

### Payload

None (zero-length payload).

### Failure Reasons

* Room does not exist
* Room is full (playerCount >= maxPlayers)
* Room is not in `Waiting` or `Countdown` state
* Max instances limit reached (for create requests)

### Example Build

```cpp
std::vector<std::uint8_t> buildJoinFailedPacket(std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType  = static_cast<std::uint8_t>(MessageType::LobbyJoinFailed);
    hdr.sequenceId   = sequence;
    hdr.payloadSize  = 0;
    hdr.originalSize = 0;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    return packet;
}
```

***

## **11. Typical Client Flow**

### Flow 1: List and Join Existing Room

```
1. Client → Lobby: LOBBY_LIST_ROOMS
2. Lobby → Client: LOBBY_ROOM_LIST (with N rooms)
3. Client selects room ID from list
4. Client → Lobby: LOBBY_JOIN_ROOM (roomId)
5. Lobby → Client: LOBBY_JOIN_SUCCESS (roomId, port) OR LOBBY_JOIN_FAILED
6. If success: Client connects to game instance at specified port
7. Client → Instance: CLIENT_HELLO (game protocol)
8. [Continue with standard game handshake]
```

### Flow 2: Create New Room

```
1. Client → Lobby: LOBBY_CREATE_ROOM
2. Lobby → Client: LOBBY_ROOM_CREATED (roomId, port) OR LOBBY_JOIN_FAILED
3. If success: Client connects to game instance at specified port
4. Client → Instance: CLIENT_HELLO (game protocol)
5. [Continue with standard game handshake]
```

***

## **12. Sequence Numbers**

Both client and lobby maintain independent sequence counters:

```cpp
std::uint16_t nextSequence_{0};
```

### Client Side

```cpp
std::uint16_t seq = nextSequence_++;
auto packet = buildListRoomsPacket(seq);
```

### Lobby Side

```cpp
// Echo client's sequence in response
auto packet = buildRoomListPacket(rooms, hdr.sequence);
```

Sequence numbers are used for:
* Matching requests with responses
* Detecting duplicate packets
* Debugging packet flow

***

## **13. Error Handling**

### Malformed Packets

Invalid packets are silently dropped:

```cpp
if (size < PacketHeader::kSize) {
    return; // Too small
}

if (!PacketHeader::verifyCrc(data, size)) {
    return; // CRC mismatch
}
```

### Timeouts

Clients should implement timeouts for lobby requests:

```cpp
std::chrono::milliseconds timeout = std::chrono::seconds(5);
auto response = sendAndWaitForResponse(packet, MessageType::LobbyRoomList, timeout);
if (!response) {
    // Timeout: lobby not responding
}
```

### Retries

Clients may retry failed requests:

```cpp
for (int attempt = 0; attempt < 3; ++attempt) {
    auto result = requestRoomList();
    if (result.has_value()) {
        break;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

***

## **14. Security Considerations**

### Rate Limiting

Lobby server should implement rate limiting per client IP to prevent abuse:

```cpp
if (clientRequestRate > MAX_REQUESTS_PER_SECOND) {
    // Drop packet or send error
}
```

### Input Validation

All fields must be validated:

```cpp
if (roomId == 0 || roomId > MAX_ROOM_ID) {
    // Invalid room ID
}

if (size != expectedSize) {
    // Malformed packet
}
```

### CRC Verification

Always verify CRC32 before processing:

```cpp
if (!PacketHeader::verifyCrc(data, size)) {
    Logger::warn("[Lobby] CRC mismatch from " + endpoint);
    return;
}
```

***

## **15. Implementation Files**

### Server Side

* **Header**: `server/include/lobby/LobbyPackets.hpp`
* **Implementation**: `server/src/LobbyPackets.cpp`
* **Tests**: `tests/server/lobby/LobbyPacketsTests.cpp`

### Client Side

* **Header**: `client/include/network/LobbyPackets.hpp`
* **Implementation**: `client/src/network/LobbyPackets.cpp`
* **Tests**: `tests/client/network/LobbyPacketsTests.cpp`

***

## **16. Testing**

### Unit Tests

Located in `tests/client/network/LobbyPacketsTests.cpp`:

* Build and parse round-trip tests
* Edge cases (empty room list, max rooms)
* Malformed packet handling
* Sequence number handling

### Integration Tests

Test full lobby workflow:
* Client connects to lobby
* Lists rooms successfully
* Creates room and receives port
* Joins room and receives port
* Handles failures gracefully

***

## **17. Related Documentation**

* [Multi-Instance Architecture](../global-overview/multi-instance-architecture.md) - Overall system design
* [Lobby System](../server/lobby-system.md) - Lobby server implementation
* [Game Instance Management](../server/game-instance-management.md) - Instance lifecycle
* [Packet Header](../server/networking/packet-header.md) - Base packet structure

***
