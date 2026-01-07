# Game Instance Management

The **GameInstanceManager** is responsible for creating, tracking, and destroying independent game instances.\
Each instance represents an isolated game session with its own ECS simulation, UDP port, and networking threads.

This page explains how instances are created, managed, and destroyed, as well as the thread-safety mechanisms that ensure stable concurrent operation.

***

## **1. Overview**

The `GameInstanceManager` maintains a collection of active `GameInstance` objects.\
Each instance:

* Runs independently on its own UDP port
* Has its own ECS registry
* Manages its own networking threads
* Operates completely isolated from other instances

The manager ensures:
* Unique room IDs are assigned
* Port conflicts are avoided
* Maximum instance limits are enforced
* Thread-safe access to instances

***

## **2. GameInstanceManager Architecture**

### File Location

* **Header**: `server/include/game/GameInstanceManager.hpp`
* **Implementation**: `server/src/GameInstanceManager.cpp`

### Constructor

```cpp
GameInstanceManager(std::uint16_t basePort,        // Base port for instances
                    std::uint32_t maxInstances,    // Max concurrent instances
                    std::atomic<bool>& runningFlag, // Shared shutdown flag
                    bool enableTui = false,        // Enable TUI for instances
                    bool enableAdmin = false);     // Enable admin panel
```

### Key Members

```cpp
std::uint16_t basePort_;                  // Base port (e.g., 8081)
std::uint32_t maxInstances_;              // Maximum allowed instances
std::uint32_t nextRoomId_{1};             // Auto-incrementing room ID

std::atomic<bool>* running_;              // Shared shutdown flag
bool enableTui_;                          // Enable TUI for instances
bool enableAdmin_;                        // Enable admin panel

mutable std::mutex instancesMutex_;       // Protects instances_ map
std::map<uint32_t, unique_ptr<GameInstance>> instances_;
```

***

## **3. Instance Creation**

### Creating a New Instance

```cpp
std::optional<std::uint32_t> createInstance();
```

**Returns**:
* `std::optional<uint32_t>` containing the room ID if successful
* `std::nullopt` if creation failed

**Failure Reasons**:
* Maximum instance limit reached
* Instance failed to start (port already in use, level not found, etc.)

### Creation Process

**Step 1: Check Capacity**

```cpp
std::lock_guard<std::mutex> lock(instancesMutex_);

if (instances_.size() >= maxInstances_) {
    Logger::warn("[InstanceManager] Cannot create instance: max instances reached");
    return std::nullopt;
}
```

**Step 2: Assign Room ID and Port**

```cpp
std::uint32_t roomId = nextRoomId_++;
std::uint16_t instancePort = basePort_ + static_cast<std::uint16_t>(roomId);
```

**Port Formula**:
```
instancePort = basePort + roomId
```

**Example**:
* Base port: 8081
* Room ID 1 → Port 8081
* Room ID 2 → Port 8082
* Room ID 3 → Port 8083

**Step 3: Create GameInstance Object**

```cpp
auto instance = std::make_unique<GameInstance>(
    roomId,
    instancePort,
    *running_,
    enableTui_,
    enableAdmin_
);
```

**Step 4: Start the Instance**

```cpp
if (!instance->start()) {
    Logger::error("[InstanceManager] Failed to start instance " + std::to_string(roomId));
    return std::nullopt;
}
```

The `start()` method:
* Loads level data
* Binds UDP socket to port
* Starts receive/send/game loop threads

**Step 5: Store and Return**

```cpp
Logger::info("[InstanceManager] Created instance " + std::to_string(roomId)
             + " on port " + std::to_string(instancePort));

instances_[roomId] = std::move(instance);
return roomId;
```

***

## **4. Instance Destruction**

### Destroying an Instance

```cpp
void destroyInstance(std::uint32_t roomId);
```

Stops and removes an instance from the manager.

### Destruction Process

**Step 1: Find Instance**

```cpp
std::lock_guard<std::mutex> lock(instancesMutex_);

auto it = instances_.find(roomId);
if (it == instances_.end()) {
    Logger::warn("[InstanceManager] Attempt to destroy non-existent instance");
    return;
}
```

**Step 2: Stop the Instance**

```cpp
it->second->stop();
```

The `stop()` method:
* Sets running flag to false
* Joins all networking threads
* Closes UDP socket
* Cleans up ECS registry

**Step 3: Remove from Map**

```cpp
instances_.erase(it);
Logger::info("[InstanceManager] Destroyed instance " + std::to_string(roomId));
```

The `unique_ptr` automatically deallocates the instance.

***

## **5. Cleanup**

### Automatic Cleanup

```cpp
void cleanupEmptyInstances();
```

Called periodically by the lobby's cleanup thread.

**Purpose**:
* Remove instances with no players
* Remove instances that have finished
* Free resources and ports

**Implementation**:

```cpp
std::lock_guard<std::mutex> lock(instancesMutex_);

for (auto it = instances_.begin(); it != instances_.end(); ) {
    if (it->second->isEmpty() || it->second->isFinished()) {
        Logger::info("[InstanceManager] Cleaning up empty instance "
                     + std::to_string(it->first));
        it->second->stop();
        it = instances_.erase(it);
    } else {
        ++it;
    }
}
```

**Cleanup Conditions**:
* `isEmpty()` → No players connected
* `isFinished()` → Game completed (level finished or all players dead)

***

## **6. Instance Access**

### Getting an Instance

```cpp
GameInstance* getInstance(std::uint32_t roomId);
```

Returns raw pointer to instance, or `nullptr` if not found.

**Usage**:
```cpp
auto* instance = instanceManager.getInstance(roomId);
if (instance) {
    size_t playerCount = instance->getPlayerCount();
    // ...
}
```

**Thread Safety**:
* The manager's mutex is **not** held during instance access
* The caller must ensure the instance is not destroyed during use
* For safe access, use short-lived operations

### Checking Instance Existence

```cpp
bool hasInstance(std::uint32_t roomId) const;
```

Thread-safe check if room ID exists.

```cpp
if (instanceManager.hasInstance(roomId)) {
    // Instance exists
}
```

***

## **7. Instance Queries**

### Get Instance Count

```cpp
std::size_t getInstanceCount() const;
```

Returns number of active instances.

```cpp
size_t active = instanceManager.getInstanceCount();
Logger::info("Active instances: " + std::to_string(active));
```

### Get Max Instances

```cpp
std::uint32_t getMaxInstances() const;
```

Returns configured maximum instance limit.

### Get All Room IDs

```cpp
std::vector<std::uint32_t> getAllRoomIds() const;
```

Returns list of all active room IDs.

```cpp
auto roomIds = instanceManager.getAllRoomIds();
for (uint32_t roomId : roomIds) {
    Logger::info("Active room: " + std::to_string(roomId));
}
```

***

## **8. GameInstance Class**

Each instance is a `GameInstance` object:

### File Location

* **Header**: `server/include/game/GameInstance.hpp`
* **Implementation**: `server/src/GameInstance.cpp`

### Key Responsibilities

* **ECS Simulation**: Run game loop at 60 FPS
* **Networking**: Receive input, send snapshots
* **Player Management**: Track connected players
* **Level Management**: Load and execute level data
* **State Reporting**: Report player count and status

### Constructor

```cpp
GameInstance(std::uint32_t roomId,
             std::uint16_t port,
             std::atomic<bool>& runningFlag,
             bool enableTui = false,
             bool enableAdmin = false);
```

### Key Methods

```cpp
bool start();                    // Start networking threads and load level
void stop();                     // Stop all threads and cleanup
bool isEmpty() const;            // True if no players connected
bool isFinished() const;         // True if game completed
size_t getPlayerCount() const;   // Number of connected players
uint16_t getPort() const;        // Instance UDP port
```

### Threading

Each instance runs **three threads**:

1. **Receive Thread** - Listens for client input on instance port
2. **Game Loop Thread** - Runs ECS simulation at 60 FPS
3. **Send Thread** - Sends delta-state snapshots to clients

These are identical to the original single-instance server architecture.

***

## **9. Port Management**

### Port Allocation Strategy

**Sequential Allocation**:
```
Port = basePort + roomId
```

**Advantages**:
* Simple and predictable
* No complex port tracking needed
* Easy to debug (room ID = port offset)

**Limitations**:
* Port range must accommodate all instances
* Ports not reused until room ID wraps around
* Must configure firewall for entire range

### Port Range Calculation

```
Port Range = [basePort, basePort + maxInstances - 1]
```

**Example**:
* Base port: 8081
* Max instances: 100
* Port range: 8081 - 8180

### Port Conflicts

Port conflicts are detected during `instance->start()`:

```cpp
if (!instance->start()) {
    // Binding failed (port in use, permission denied, etc.)
    return std::nullopt;
}
```

***

## **10. Thread Safety**

### Mutex Protection

All operations on `instances_` map are protected by `instancesMutex_`:

```cpp
mutable std::mutex instancesMutex_;
```

### Thread-Safe Operations

**Creation**:
```cpp
std::lock_guard<std::mutex> lock(instancesMutex_);
instances_[roomId] = std::move(instance);
```

**Destruction**:
```cpp
std::lock_guard<std::mutex> lock(instancesMutex_);
instances_.erase(roomId);
```

**Queries**:
```cpp
std::lock_guard<std::mutex> lock(instancesMutex_);
return instances_.size();
```

### Concurrent Access

Multiple threads can safely:
* Create instances (lobby thread)
* Destroy instances (cleanup thread)
* Query instance count (lobby thread)
* Access instances by ID (lobby thread)

***

## **11. Resource Management**

### Memory Management

* Instances stored as `std::unique_ptr<GameInstance>`
* Automatic deallocation when erased from map
* Each instance allocates its own ECS registry

### Thread Management

* Each instance spawns 3 threads
* Total threads = 3 × N (where N = instance count)
* Threads joined during `stop()` before destruction

### Socket Management

* Each instance binds one UDP socket
* Sockets closed during `stop()`
* Ports freed for OS reuse after socket closure

***

## **12. Error Handling**

### Max Instances Reached

```cpp
if (instances_.size() >= maxInstances_) {
    Logger::warn("[InstanceManager] Cannot create instance: max instances reached");
    return std::nullopt;
}
```

Client receives `LOBBY_JOIN_FAILED` response.

### Instance Start Failure

```cpp
if (!instance->start()) {
    Logger::error("[InstanceManager] Failed to start instance " + std::to_string(roomId));
    return std::nullopt;
}
```

Common causes:
* Port already in use
* Level file not found: `server/assets/levels/level_1.json`
* Insufficient permissions
* Socket creation failure

### Destroy Non-Existent Instance

```cpp
if (it == instances_.end()) {
    Logger::warn("[InstanceManager] Attempt to destroy non-existent instance");
    return;
}
```

This is a no-op and logs a warning.

***

## **13. Configuration**

### Recommended Settings

**Small Server** (1-10 concurrent games):
```cpp
uint16_t basePort = 8081;
uint32_t maxInstances = 10;
```

**Medium Server** (10-50 concurrent games):
```cpp
uint16_t basePort = 8081;
uint32_t maxInstances = 50;
```

**Large Server** (50-100 concurrent games):
```cpp
uint16_t basePort = 8081;
uint32_t maxInstances = 100;
```

### System Considerations

**Thread Limit**:
* 100 instances = 300 instance threads + 2 lobby threads = 302 threads
* Ensure system `ulimit -u` is sufficient

**Port Limit**:
* 100 instances = 100 UDP ports
* Ensure firewall allows port range
* Check system maximum socket limit

**Memory Usage**:
* Each instance: ~10-50 MB (depends on ECS registry size)
* 100 instances: ~1-5 GB total

***

## **14. Logging**

The manager logs all major events:

```
[InstanceManager] Initialized with max instances: 100
[InstanceManager] Created instance 1 on port 8081
[InstanceManager] Created instance 2 on port 8082
[InstanceManager] Cleaning up empty instance 1
[InstanceManager] Destroyed instance 1
[InstanceManager] Cannot create instance: max instances (100) reached
```

***

## **15. Testing**

Tests located in: `tests/server/game/GameInstanceManagerTests.cpp`

**Test Coverage**:
* Create first instance
* Create multiple instances
* Create up to max instances
* Cannot exceed max instances
* Destroy instance
* Recreate after destroy
* Port allocation correctness
* Cleanup empty instances
* Thread-safe concurrent creation

***

## **16. Best Practices**

### Do

✅ Set reasonable `maxInstances` based on server capacity\
✅ Configure firewall to allow the full port range\
✅ Monitor instance count and resource usage\
✅ Use `cleanupEmptyInstances()` periodically\
✅ Handle `std::nullopt` returns from `createInstance()`

### Don't

❌ Set `maxInstances` too high without testing\
❌ Hold `getInstance()` pointers for extended periods\
❌ Call `destroyInstance()` from within an instance thread\
❌ Forget to check return value of `createInstance()`\
❌ Use room IDs without checking existence first

***

## **17. Related Documentation**

* [Multi-Instance Architecture](../global-overview/multi-instance-architecture.md) - Overall system design
* [Lobby System](lobby-system.md) - How lobby coordinates with instance manager
* [Server Architecture Overview](architecture-overview.md) - Single-instance architecture that each GameInstance implements

***
