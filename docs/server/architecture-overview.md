#Architecture Overview

The server architecture is designed to maintain a **stable, deterministic, and authoritative simulation** of the R-Type game world.\
It runs independently from the clients and ensures that all players receive a consistent and validated version of the game state.

This page explains the **overall structure** of the server, how each subsystem interacts, and how the ECS integrates with the multithreaded networking pipeline.

***

## **1. High-Level Architecture**

The server is composed of **three major subsystems**:

#### **1. Networking**

Handles:

* Receiving player inputs over UDP
* Sending state updates to clients
* Managing client connections/disconnections
* Validating packets

Networking runs on **dedicated threads** to avoid blocking the simulation loop.

***

#### **2. Game Logic (ECS Simulation Loop)**

Runs at a **fixed deterministic frequency (60 FPS)**.

It applies all gameplay rules using ECS systems:

* Input processing
* Movement
* Monster AI
* Missile behavior
* Spawn logic
* Collision detection
* Damage application
* Entity destruction
* Snapshot generation
* Event packets: player disconnects, entity spawns, entity destructions

The ECS Registry contains **all authoritative gameplay components**.

***

#### **3. Delta-State System**

At the end of every tick, the server:

1. Compares current components with the previous tick
2. Generates a **compact delta snapshot**
3. Sends it to all clients
4. Caches it for potential retransmission

This ensures:

* Low bandwidth usage
* Fast updates
* Perfect synchronization between players

***

## **2. Diagram – Server Global Architecture**

<figure><img src="../assets/server_EN.png" alt=""><figcaption></figcaption></figure>

This design ensures:

* The simulation **never blocks**
* Networking remains responsive
* Packets do not delay gameplay
* ECS systems always run in deterministic order

***

## **3. Detailed Data Flow**

### **Step 1 - Input Reception (Receive Thread)**

1. Client sends input packet
2. UDP thread receives raw datagram
3. Packet is validated (size, type, checksum, etc.)
4. Extracted input is assigned a `sequenceId`
5. Input is pushed into a **thread-safe queue** for the main loop

This decouples untrusted network I/O from gameplay logic.

***

### **Step 2 - ECS Simulation (Main Loop)**

Runs at a fixed timestep of **16.66 ms (60 FPS)**:

1. Pull all available inputs
2. Convert inputs into ECS components (`PlayerInputComponent`)
3. Run systems in order:
   * PlayerInputSystem
   * MonsterAISystem
   * MovementSystem
   * CollisionSystem
   * DamageSystem
   * DestructionSystem
   * SpawnSystem
4. Resolve entity creation/destruction
5. Produce a compact **delta-state** structure

All computations happen locally on the server and depend solely on ECS data.

***

### **Step 3 - Snapshot Sending (Send Thread)**

The server maintains a **snapshot buffer** produced each frame.

The send thread:

* Reads the latest snapshot
* Serializes it into binary packets
* Sends it to each client as fast as possible
* Updates per-client sequence counters
* Discards old snapshots

The server **never waits** for slow clients.

***

## **4. ECS Integration in the Server**

The server uses ECS exclusively for **gameplay simulation**.

It holds:

#### Pure gameplay components:

* Position and rotation
* Velocity
* Hitboxes
* Health
* Monster AI state
* Missile data
* Lifetime timers
* Player input state

#### Gameplay systems:

* Movement
* AI
* Collisions
* Damage
* Spawning
* Destruction
* Networking delta-state preparation

No rendering or animation systems exist on the server.

***

## **5. Design Goals**

The architecture was designed with the following goals:

#### **Determinism**

Same input → same output.\
Essential for multiplayer syncing and debugging.

#### **High performance**

Strictly optimized ECS iteration loops.

#### **Non-blocking networking**

Receive/send threads allow main loop to run uninterrupted.

#### **Robustness**

Invalid packets, disconnects, and client crashes never affect the simulation.

#### **Scalable**

Server can handle:

* Many entities
* Many missiles
* Many monsters\
  with minimal CPU cost.
