# Multi-Instance Architecture

The R-Type server has been designed to support **multiple concurrent game instances**, allowing many independent game sessions to run simultaneously on the same server infrastructure.

This architecture enables:

* **Horizontal scalability** → Many rooms running in parallel
* **Isolation** → Each game instance is independent
* **Efficient resource usage** → Instances share the same process
* **Dynamic creation/destruction** → Rooms created and destroyed on demand
* **Centralized lobby** → Single entry point for all clients

This document provides a high-level overview of how the multi-instance system works and how the lobby, game instances, and clients interact.

***

## **1. Overview**

In a traditional single-instance server, one ECS registry runs the entire game world, and all players connect to a single UDP port.

In the **multi-instance architecture**, the server runs:

1. **One Lobby Server** (single UDP port)
   * Manages room listings
   * Handles room creation requests
   * Dispatches clients to game instances

2. **Multiple Game Instances** (one UDP port per instance)
   * Each instance runs its own isolated ECS simulation
   * Each instance has its own UDP port
   * Each instance operates independently

### Architecture Diagram

```
                    ┌─────────────────┐
                    │   Lobby Server  │
                    │   (Port 8080)   │
                    └────────┬────────┘
                             │
          ┌──────────────────┼──────────────────┐
          │                  │                  │
     ┌────▼────┐        ┌────▼────┐       ┌────▼────┐
     │ Instance│        │ Instance│       │ Instance│
     │ Room 1  │        │ Room 2  │       │ Room 3  │
     │Port 8081│        │Port 8082│       │Port 8083│
     └─────────┘        └─────────┘       └─────────┘
         │                   │                  │
    ┌────┴────┐         ┌────┴────┐       ┌────┴────┐
    │Player 1 │         │Player 3 │       │Player 5 │
    │Player 2 │         │Player 4 │       │Player 6 │
    └─────────┘         └─────────┘       └─────────┘
```

***

## **2. Client Connection Flow**

When a client wants to join a game, the following flow occurs:

### Step 1: Connect to Lobby

```
Client → Lobby Server (Port 8080)
- List available rooms
- Create new room OR join existing room
```

### Step 2: Lobby Response

The lobby server responds with:

* **Room List**: List of available game instances with their state and player count
* **Room Created**: New instance created, here's the room ID and port
* **Join Success**: Room exists, here's the port to connect to

### Step 3: Connect to Game Instance

```
Client → Game Instance (Port 8081, 8082, ...)
- Standard game handshake (CLIENT_HELLO, SERVER_HELLO, etc.)
- Join the ECS simulation
- Start receiving game state updates
```

### Step 4: Play the Game

Once connected to a game instance:

* Client sends input packets to the instance's UDP port
* Instance sends delta-state snapshots back
* Instance runs the full ECS simulation independently

### Full Flow Diagram

```
┌────────┐                  ┌──────────┐                 ┌──────────┐
│ Client │                  │  Lobby   │                 │ Instance │
└────┬───┘                  └────┬─────┘                 └────┬─────┘
     │                           │                            │
     │ LOBBY_LIST_ROOMS          │                            │
     ├──────────────────────────>│                            │
     │                           │                            │
     │ LOBBY_ROOM_LIST (rooms)   │                            │
     │<──────────────────────────┤                            │
     │                           │                            │
     │ LOBBY_CREATE_ROOM         │                            │
     ├──────────────────────────>│                            │
     │                           │ createInstance()           │
     │                           ├───────────────────────────>│
     │                           │                            │ (starts)
     │ LOBBY_ROOM_CREATED        │                            │
     │ (roomId=1, port=8081)     │                            │
     │<──────────────────────────┤                            │
     │                           │                            │
     │ CLIENT_HELLO                                           │
     ├────────────────────────────────────────────────────────>│
     │                                                         │
     │ SERVER_HELLO                                           │
     │<────────────────────────────────────────────────────────┤
     │                                                         │
     │ CLIENT_JOIN_REQUEST                                    │
     ├────────────────────────────────────────────────────────>│
     │                                                         │
     │ SERVER_JOIN_ACCEPT                                     │
     │<────────────────────────────────────────────────────────┤
     │                                                         │
     │ [Game loop: input + snapshots]                         │
     │<───────────────────────────────────────────────────────>│
```

***

## **3. Server-Side Components**

The multi-instance architecture introduces three major server-side components:

### 3.1 LobbyServer

**File**: `server/include/lobby/LobbyServer.hpp`

**Responsibilities**:
* Listen on lobby UDP port (default: 8080)
* Handle lobby protocol messages (`LOBBY_LIST_ROOMS`, `LOBBY_CREATE_ROOM`, `LOBBY_JOIN_ROOM`)
* Coordinate with `GameInstanceManager` and `LobbyManager`
* Send room listings and connection info to clients
* Run cleanup thread to remove empty/finished instances

**Threads**:
* **Receive Thread**: Listens for incoming lobby packets
* **Cleanup Thread**: Periodically removes finished game instances

### 3.2 GameInstanceManager

**File**: `server/include/game/GameInstanceManager.hpp`

**Responsibilities**:
* Create new game instances dynamically
* Assign unique room IDs and UDP ports
* Track all active instances
* Destroy instances when no longer needed
* Enforce maximum instance limit
* Thread-safe instance access

**Key Operations**:
* `createInstance()` → Spawns new `GameInstance`, returns room ID
* `destroyInstance(roomId)` → Stops and removes instance
* `getInstance(roomId)` → Returns pointer to running instance
* `cleanupEmptyInstances()` → Removes finished instances

### 3.3 LobbyManager

**File**: `server/include/lobby/LobbyManager.hpp`

**Responsibilities**:
* Track room metadata (player count, state, port)
* Provide room listings for lobby queries
* Update room state as instances change
* Thread-safe room data access

**Room States**:
* `Waiting` → Room created, waiting for players
* `Countdown` → Countdown before game starts
* `Playing` → Game in progress
* `Finished` → Game completed

### 3.4 GameInstance

**File**: `server/include/game/GameInstance.hpp`

**Responsibilities**:
* Run a full isolated game simulation
* Manage its own ECS registry
* Run its own networking threads (receive, game loop, send)
* Load level data independently
* Track connected players
* Report status back to lobby

**Each instance**:
* Has its own UDP socket on a unique port
* Runs in its own set of threads
* Is completely independent from other instances
* Follows the same architecture as the original single-instance server

***

## **4. Client-Side Components**

### 4.1 LobbyConnection

**File**: `client/include/network/LobbyConnection.hpp`

**Responsibilities**:
* Connect to lobby server
* Request room listings
* Create new rooms
* Join existing rooms
* Handle lobby protocol messages
* Extract game instance connection info

**Key Operations**:
* `requestRoomList()` → Returns list of available rooms
* `createRoom()` → Requests new instance, returns room ID and port
* `joinRoom(roomId)` → Requests to join, returns port

### 4.2 LobbyMenu (UI)

**File**: `client/include/menu/LobbyMenu.hpp`

**Responsibilities**:
* Display room listings
* Handle user input for room selection/creation
* Trigger connection to game instances
* Provide UI feedback

***

## **5. Protocol Overview**

The lobby uses a dedicated protocol built on top of the existing UDP packet structure.

### Lobby Message Types

| Message Type | Direction | Purpose |
|-------------|-----------|---------|
| `LOBBY_LIST_ROOMS` | Client → Lobby | Request list of available rooms |
| `LOBBY_ROOM_LIST` | Lobby → Client | Response with room data |
| `LOBBY_CREATE_ROOM` | Client → Lobby | Request to create new room |
| `LOBBY_ROOM_CREATED` | Lobby → Client | Confirmation with room ID and port |
| `LOBBY_JOIN_ROOM` | Client → Lobby | Request to join existing room |
| `LOBBY_JOIN_SUCCESS` | Lobby → Client | Join approved, here's the port |
| `LOBBY_JOIN_FAILED` | Lobby → Client | Room full or doesn't exist |

For detailed packet structure, see [Lobby Protocol](../protocol/lobby-protocol.md).

***

## **6. Port Allocation**

Ports are allocated sequentially based on room IDs:

```cpp
instancePort = basePort + roomId
```

**Example**:
* Lobby Server: Port 8080
* Base Port: 8081
* Room 1: Port 8081
* Room 2: Port 8082
* Room 3: Port 8083
* ...

This ensures:
* No port conflicts between instances
* Predictable port assignment
* Simple port management

***

## **7. Instance Lifecycle**

### Creation

1. Client sends `LOBBY_CREATE_ROOM` to lobby
2. Lobby calls `instanceManager.createInstance()`
3. `GameInstanceManager` creates new `GameInstance`
4. Instance starts its networking threads
5. Instance loads level data
6. Room added to `LobbyManager` with `Waiting` state
7. Lobby responds to client with room ID and port

### Active Game

1. Clients connect to instance's UDP port
2. Instance runs normal game loop (60 FPS)
3. Instance updates `LobbyManager` with player count
4. Instance transitions through states: `Waiting` → `Countdown` → `Playing`

### Destruction

1. Game finishes or all players disconnect
2. Instance state set to `Finished`
3. Cleanup thread detects empty/finished instance
4. `instanceManager.destroyInstance(roomId)` called
5. Instance threads stopped
6. Room removed from `LobbyManager`
7. Port freed for reuse

***

## **8. Threading Model**

Each game instance runs **independently** with its own set of threads:

### Per-Instance Threads

* **Receive Thread**: Listens for client input on instance port
* **Game Loop Thread**: Runs ECS simulation at 60 FPS
* **Send Thread**: Sends delta-state snapshots to clients

### Lobby Threads

* **Receive Thread**: Handles lobby protocol messages
* **Cleanup Thread**: Removes finished instances periodically

### Total Thread Count

```
Total Threads = 2 (lobby threads) + 3 × N (instance threads)
```

Where N = number of active game instances.

**Example**:
* 5 active instances → 2 + 15 = 17 threads total

***

## **9. Advantages**

### Scalability

* Multiple games run in parallel without interference
* Server can handle many concurrent sessions
* Resources distributed across instances

### Isolation

* Bug in one instance doesn't affect others
* Each game has independent state
* Players in different rooms never interact

### Flexibility

* Instances created/destroyed dynamically
* No pre-allocated resources
* Efficient use of server capacity

### Simplicity

* Each instance uses the same code as single-instance server
* No complex inter-instance communication
* Clear separation of concerns

***

## **10. Limitations**

### Port Usage

* Each instance requires a unique UDP port
* Port range must be configured appropriately
* Limited by system's maximum open sockets

### Memory Usage

* Each instance has its own ECS registry
* Memory scales linearly with instance count
* Must set reasonable `maxInstances` limit

### Thread Count

* Each instance creates 3 threads
* High instance count → many threads
* Must consider system thread limits

***

## **11. Configuration**

### Server Configuration

```cpp
std::uint16_t lobbyPort = 8080;      // Lobby server port
std::uint16_t gameBasePort = 8081;   // First game instance port
std::uint32_t maxInstances = 100;    // Maximum concurrent instances
```

### Client Configuration

```cpp
IpEndpoint lobbyEndpoint("127.0.0.1", 8080);  // Lobby server address
```

***

## **12. What's Next**

For more detailed information, see:

* **[Lobby System](../server/lobby-system.md)** - Detailed lobby server architecture
* **[Game Instance Management](../server/game-instance-management.md)** - Instance lifecycle and management
* **[Lobby Protocol](../protocol/lobby-protocol.md)** - Complete protocol specification
* **[Server Architecture Overview](../server/architecture-overview.md)** - Updated server architecture

***
