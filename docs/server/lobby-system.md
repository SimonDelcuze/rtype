# Lobby System

The **Lobby System** is the central coordination point for the multi-instance server architecture.\
It handles client discovery, room creation, room joining, and dispatching clients to the appropriate game instances.

This page explains the lobby server's architecture, threading model, packet handling, and integration with other components.

***

## **1. Overview**

The lobby server acts as a **front door** for all clients:

* Clients connect to the lobby first
* Lobby provides room listings
* Lobby creates new game instances on demand
* Lobby routes clients to existing instances
* Lobby tracks all active rooms

The lobby itself **does not run game logic**.\
It only coordinates connections and manages metadata about game instances.

***

## **2. LobbyServer Architecture**

### File Location

* **Header**: `server/include/lobby/LobbyServer.hpp`
* **Implementation**: `server/src/LobbyServer.cpp`

### Constructor

```cpp
LobbyServer(std::uint16_t lobbyPort,        // Port to listen on (e.g., 8080)
            std::uint16_t gameBasePort,     // Base port for game instances
            std::uint32_t maxInstances,     // Max concurrent instances
            std::atomic<bool>& runningFlag, // Shared shutdown flag
            bool enableTui = false,         // Enable TUI for instances
            bool enableAdmin = false);      // Enable admin panel for instances
```

### Key Members

```cpp
// Network
UdpSocket lobbySocket_;                    // Lobby UDP socket
std::uint16_t lobbyPort_;                  // Lobby listening port

// Instance management
GameInstanceManager instanceManager_;       // Creates/destroys instances
LobbyManager lobbyManager_;                // Tracks room metadata

// Session management
std::unordered_map<std::string, ClientSession> lobbySessions_;
mutable std::mutex sessionsMutex_;

// Threading
std::thread receiveWorker_;                // Receives lobby packets
std::thread cleanupWorker_;                // Cleans up finished instances
std::atomic<bool> receiveRunning_;
std::atomic<bool>* running_;               // Shared shutdown flag
```

***

## **3. Threading Model**

The lobby server runs two dedicated threads:

### 3.1 Receive Thread

**Function**: `receiveThread()`

**Responsibilities**:
* Listen for incoming UDP packets on lobby port
* Validate packet headers
* Extract message type
* Dispatch to appropriate handler
* Send responses back to clients

**Loop**:
```cpp
while (receiveRunning_) {
    auto [data, size, from] = lobbySocket_.receiveFrom();
    if (size > 0) {
        handlePacket(data, size, from);
    }
}
```

**Handler Dispatch**:
```cpp
switch (hdr.messageType) {
    case MessageType::LOBBY_LIST_ROOMS:
        handleLobbyListRooms(hdr, from);
        break;
    case MessageType::LOBBY_CREATE_ROOM:
        handleLobbyCreateRoom(hdr, from);
        break;
    case MessageType::LOBBY_JOIN_ROOM:
        handleLobbyJoinRoom(hdr, data, size, from);
        break;
}
```

### 3.2 Cleanup Thread

**Function**: `cleanupThread()`

**Responsibilities**:
* Periodically check for empty or finished game instances
* Call `instanceManager_.cleanupEmptyInstances()`
* Remove corresponding entries from `lobbyManager_`

**Loop**:
```cpp
while (*running_) {
    std::this_thread::sleep_for(std::chrono::seconds(30));
    instanceManager_.cleanupEmptyInstances();
}
```

This ensures resources are freed and ports are recycled when instances are no longer needed.

***

## **4. Packet Handlers**

### 4.1 List Rooms Handler

**Trigger**: Client sends `LOBBY_LIST_ROOMS`

**Function**: `handleLobbyListRooms(const PacketHeader& hdr, const IpEndpoint& from)`

**Steps**:
1. Extract sequence number from header
2. Call `lobbyManager_.listRooms()` to get all active rooms
3. Build `LOBBY_ROOM_LIST` packet with room data
4. Send response to client

**Response Format**:
```
[PacketHeader: LOBBY_ROOM_LIST]
[uint32_t: roomCount]
For each room:
  [uint32_t: roomId]
  [uint16_t: port]
  [uint8_t:  state]
  [uint32_t: playerCount]
  [uint32_t: maxPlayers]
```

**Example**:
```cpp
auto rooms = lobbyManager_.listRooms();
auto packet = buildRoomListPacket(rooms, hdr.sequence);
sendPacket(packet, from);
```

***

### 4.2 Create Room Handler

**Trigger**: Client sends `LOBBY_CREATE_ROOM`

**Function**: `handleLobbyCreateRoom(const PacketHeader& hdr, const IpEndpoint& from)`

**Steps**:
1. Call `instanceManager_.createInstance()` to spawn new instance
2. If successful:
   * Get room ID and port
   * Add room to `lobbyManager_` with `Waiting` state
   * Build `LOBBY_ROOM_CREATED` packet
   * Send response to client
3. If failed (max instances reached):
   * Build `LOBBY_JOIN_FAILED` packet
   * Send error response

**Response Format (Success)**:
```
[PacketHeader: LOBBY_ROOM_CREATED]
[uint32_t: roomId]
[uint16_t: port]
```

**Example**:
```cpp
auto roomId = instanceManager_.createInstance();
if (roomId.has_value()) {
    uint16_t port = gameBasePort_ + roomId.value();
    lobbyManager_.addRoom(roomId.value(), port);
    auto packet = buildRoomCreatedPacket(roomId.value(), port, hdr.sequence);
    sendPacket(packet, from);
}
```

***

### 4.3 Join Room Handler

**Trigger**: Client sends `LOBBY_JOIN_ROOM` with room ID

**Function**: `handleLobbyJoinRoom(const PacketHeader& hdr, const uint8_t* data, size_t size, const IpEndpoint& from)`

**Steps**:
1. Extract room ID from packet payload
2. Check if room exists: `lobbyManager_.getRoomInfo(roomId)`
3. If room exists:
   * Verify room is not full
   * Verify room is in `Waiting` or `Countdown` state
   * Build `LOBBY_JOIN_SUCCESS` packet with port
   * Send response to client
4. If room doesn't exist or is full:
   * Build `LOBBY_JOIN_FAILED` packet
   * Send error response

**Request Format**:
```
[PacketHeader: LOBBY_JOIN_ROOM]
[uint32_t: roomId]
```

**Response Format (Success)**:
```
[PacketHeader: LOBBY_JOIN_SUCCESS]
[uint32_t: roomId]
[uint16_t: port]
```

**Example**:
```cpp
uint32_t roomId = /* extract from payload */;
auto roomInfo = lobbyManager_.getRoomInfo(roomId);
if (roomInfo.has_value()) {
    auto packet = buildJoinSuccessPacket(roomId, roomInfo->port, hdr.sequence);
    sendPacket(packet, from);
}
```

***

## **5. Integration with GameInstanceManager**

The lobby delegates all instance lifecycle operations to `GameInstanceManager`:

### Creating Instances

```cpp
auto roomId = instanceManager_.createInstance();
// Returns std::optional<uint32_t>
// nullopt if max instances reached or start failed
```

The `GameInstanceManager`:
1. Checks if `maxInstances` limit is reached
2. Assigns next available room ID
3. Calculates UDP port: `basePort + roomId`
4. Creates `GameInstance` object
5. Calls `instance->start()` to start networking threads
6. Returns room ID if successful

### Destroying Instances

```cpp
instanceManager_.destroyInstance(roomId);
```

This:
1. Stops the instance's networking threads
2. Cleans up the ECS registry
3. Removes the instance from the manager's map
4. Frees the UDP port for reuse

### Checking Instance Status

```cpp
auto* instance = instanceManager_.getInstance(roomId);
if (instance) {
    // Access instance to check player count, state, etc.
}
```

***

## **6. Integration with LobbyManager**

The lobby tracks **metadata** about rooms using `LobbyManager`:

### Adding Rooms

```cpp
lobbyManager_.addRoom(roomId, port, maxPlayers);
```

Creates a `RoomInfo` entry:
```cpp
struct RoomInfo {
    uint32_t  roomId;
    size_t    playerCount;
    size_t    maxPlayers;
    RoomState state;        // Waiting, Countdown, Playing, Finished
    uint16_t  port;
};
```

### Updating Room State

```cpp
lobbyManager_.updateRoomState(roomId, RoomState::Playing);
```

Game instances can report state changes back to the lobby.

### Listing Rooms

```cpp
auto rooms = lobbyManager_.listRooms();
// Returns vector<RoomInfo>
```

Used to respond to `LOBBY_LIST_ROOMS` requests.

### Removing Rooms

```cpp
lobbyManager_.removeRoom(roomId);
```

Called when instance is destroyed.

***

## **7. Session Management**

The lobby tracks client sessions to handle timeouts and duplicate requests:

### Session Structure

```cpp
struct ClientSession {
    IpEndpoint endpoint;
    std::chrono::steady_clock::time_point lastSeen;
    uint16_t lastSequence;
};
```

### Session Key

```cpp
std::string endpointToKey(const IpEndpoint& ep) const {
    return ep.getIp() + ":" + std::to_string(ep.getPort());
}
```

### Session Tracking

```cpp
std::lock_guard<std::mutex> lock(sessionsMutex_);
auto key = endpointToKey(from);
lobbySessions_[key] = {from, std::chrono::steady_clock::now(), hdr.sequence};
```

This allows:
* Duplicate packet detection
* Client timeout detection
* Session cleanup

***

## **8. Startup and Shutdown**

### Starting the Lobby

```cpp
bool LobbyServer::start() {
    if (!lobbySocket_.bind(lobbyPort_)) {
        Logger::error("[LobbyServer] Failed to bind to port " + std::to_string(lobbyPort_));
        return false;
    }

    receiveRunning_ = true;
    receiveWorker_ = std::thread(&LobbyServer::receiveThread, this);
    cleanupWorker_ = std::thread(&LobbyServer::cleanupThread, this);

    Logger::info("[LobbyServer] Started on port " + std::to_string(lobbyPort_));
    return true;
}
```

### Running the Lobby

```cpp
void LobbyServer::run() {
    while (*running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
```

The main thread waits while worker threads handle all logic.

### Stopping the Lobby

```cpp
void LobbyServer::stop() {
    receiveRunning_ = false;

    if (receiveWorker_.joinable()) {
        receiveWorker_.join();
    }
    if (cleanupWorker_.joinable()) {
        cleanupWorker_.join();
    }

    lobbySocket_.close();
    Logger::info("[LobbyServer] Stopped");
}
```

***

## **9. Error Handling**

### Max Instances Reached

When `createInstance()` returns `nullopt`:
```cpp
auto roomId = instanceManager_.createInstance();
if (!roomId.has_value()) {
    auto packet = buildJoinFailedPacket(hdr.sequence);
    sendPacket(packet, from);
}
```

### Room Not Found

When client tries to join non-existent room:
```cpp
auto roomInfo = lobbyManager_.getRoomInfo(roomId);
if (!roomInfo.has_value()) {
    auto packet = buildJoinFailedPacket(hdr.sequence);
    sendPacket(packet, from);
}
```

### Malformed Packets

Invalid packets are silently dropped:
```cpp
if (size < sizeof(PacketHeader)) {
    return; // Ignore malformed packet
}
```

***

## **10. Thread Safety**

### Mutex Protection

* `instanceManager_` uses internal mutex for `instances_` map
* `lobbyManager_` uses internal mutex for `rooms_` map
* `lobbySessions_` protected by `sessionsMutex_`

### Lock Ordering

To avoid deadlocks, locks are acquired in this order:
1. `sessionsMutex_`
2. `lobbyManager_` internal mutex
3. `instanceManager_` internal mutex

***

## **11. Configuration**

### Default Values

```cpp
std::uint16_t lobbyPort = 8080;        // Lobby server port
std::uint16_t gameBasePort = 8081;     // First instance port
std::uint32_t maxInstances = 100;      // Maximum concurrent instances
```

### Port Range

With base port 8081 and max 100 instances:
* Instance ports: 8081 - 8180

Ensure firewall rules allow this range.

***

## **12. Logging**

The lobby logs all major events:

* Server start/stop
* Instance creation/destruction
* Room creation/join requests
* Error conditions

Example logs:
```
[LobbyServer] Started on port 8080
[InstanceManager] Created instance 1 on port 8081
[LobbyServer] Client 127.0.0.1:54321 created room 1
[LobbyServer] Client 127.0.0.1:54322 joined room 1
[InstanceManager] Destroyed instance 1 (finished)
```

***

## **13. Testing**

Tests located in: `tests/server/lobby/LobbyManagerTests.cpp`

**Test Coverage**:
* Room creation and listing
* Room state updates
* Player count updates
* Room removal
* Concurrent access

***

## **14. Related Documentation**

* [Multi-Instance Architecture](../global-overview/multi-instance-architecture.md) - Overall system design
* [Game Instance Management](game-instance-management.md) - Instance lifecycle details
* [Lobby Protocol](../protocol/lobby-protocol.md) - Packet format specification

***
