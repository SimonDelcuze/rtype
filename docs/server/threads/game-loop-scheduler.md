# Game Loop Thread (Scheduler)

The Game Loop Thread is the core of the authoritative server simulation.\
It executes the entire gameplay logic at a fixed and deterministic rate of 60 updates per second.\
This thread consumes validated inputs, updates the ECS world, and produces delta-state snapshots to be sent to clients.

This page describes its responsibilities, execution model, and integration with the ECS architecture.

***

### **1. Purpose of the Game Loop Thread**

The Game Loop Thread is responsible for maintaining a consistent, frame-by-frame simulation of the game world.\
Its main objectives are:

* Ensure deterministic, repeatable updates independent of client behavior
* Apply all gameplay rules through ECS systems
* Process all player inputs collected by the Receive Thread
* Update positions, AI, collisions, and entity states
* Produce a compact state update for the Send Thread
* Maintain a stable fixed timestep for synchronization

This thread is never allowed to block or wait for external events.

***

### **2. Fixed Timestep Execution**

The server uses a constant timestep of 16.66 milliseconds (60 Hz).\
Each cycle consists of:

1. Reading the current time
2. Checking whether one tick interval has elapsed
3. Running one simulation update
4. Producing a delta-state snapshot
5. Sleeping or yielding until the next tick

This model ensures identical behavior across machines and allows clients to interpolate between frames.

***

### **3. Input Consumption**

At the start of each update, the Game Loop Thread drains the thread-safe input queue filled by the Receive Thread.

Steps:

1. Retrieve all pending input commands for all connected clients
2. Apply ordering using sequence IDs
3. Store validated input in `PlayerInputComponent` for each player entity

No input is discarded except stale or malformed values.\
All valid inputs received before the tick are applied during that tick.

***

### **4. System Execution Pipeline**

Once inputs are consumed, the Game Loop executes all ECS systems in a strictly defined order.\
This order guarantees both determinism and clean dependency resolution.

A typical pipeline is:

1. **PlayerInputSystem**\
   Applies movement intent, aiming, and shooting requests.
2. **MonsterAISystem**\
   Updates enemy behaviors, cooldowns, and spawns projectiles.
3. **MovementSystem**\
   Integrates velocity into positions.
4. **CollisionSystem**\
   Detects overlaps and resolves collisions.
5. **DamageSystem**\
   Converts collision results into health changes.
6. **DestructionSystem**\
   Removes entities whose health or lifetime has reached zero.
7. **SpawnSystem**\
   Creates new monsters or objects based on timers or game rules.
8. **Event Processing**\
   Processes any pending events from the Event Bus.

Each system operates exclusively on ECS data.\
There are no external calls, and no system communicates directly with another.

***

### **5. Entity and Component Life-Cycle Management**

The Game Loop Thread is the only thread allowed to:

* Create entities
* Destroy entities
* Add or remove components
* Modify core ECS state

This rule prevents race conditions and ensures all game logic happens in a synchronized, predictable environment.

Entities marked for destruction during a system update are removed at the end of the tick, after event processing.

***

### **6. Snapshot Production**

At the end of each update, the Game Loop Thread prepares a delta-state snapshot.\
This snapshot contains only the data that changed since the previous tick.

The snapshot typically includes:

* Updated transforms
* Newly created entities
* Destroyed entities
* Updated health values
* Missile creation or removal
* Player and enemy movement
* Any other state needed for client rendering

The snapshot is stored in a buffer shared with the Send Thread.\
The Game Loop Thread does not perform serialization; it only produces structured data.

***

### **7. Non-Blocking Behavior**

The Game Loop Thread does not:

* Wait for client packets
* Perform any network operations
* Wait for locks on shared resources
* Handle slow clients

All external communication is decoupled through:

* The input queue (Receive Thread → Game Loop)
* The snapshot buffer (Game Loop → Send Thread)

This ensures the simulation continues smoothly even under unstable network conditions.

***

### **8. Error Handling and Robustness**

The Game Loop Thread is designed to operate even in adverse conditions:

* Players may disconnect at any time
* Input packets may stop arriving
* Malformed inputs are ignored
* Entities may appear or disappear due to network delay

None of these conditions affect the stability of the simulation.

***

### **9. Summary**

The Game Loop Thread is the authoritative core of the server.\
It ensures:

* Deterministic simulation
* Consistent update frequency
* Correct, ordered input application
* Safe execution of all gameplay systems
* Stable snapshot production for clients
* Complete isolation from network variability

This thread defines the actual game world that all clients depend on.
