# R-Type Server Technical Documentation

## 1. Introduction

This document describes the complete technical design of the R-Type server. It follows all constraints defined in the official project specification and focuses exclusively on the server side. The objective is to provide a clear, structured, implementation-ready vision of the server architecture, networking model, internal systems, and game loop.

The server is authoritative: it controls all gameplay logic, world simulation, entity lifecycle, and event propagation. Clients render the game locally but must fully comply with server decisions.

The entire project uses a fully binary protocol transported over UDP for in-game communication, as required.

---

## 2. Server Architecture Overview

The server is designed as a modular game engine using an Entity–Component–System (ECS) architecture. ECS ensures strong decoupling between data and logic, simplifies parallelism, and allows scalable extensions.

### 2.1 Subsystem Summary

The server is composed of the following independent subsystems:

* **Core ECS**: entity registry, component storage, system scheduler.
* **Game Logic System**: applies gameplay rules, handles inputs, triggers spawns, manages AI.
* **Physics & Collision System**: updates movement, checks overlaps, resolves collisions.
* **Networking System**: receives UDP inputs, sends world updates, processes malformed packets safely.
* **Event System**: internal message bus used for decoupled communication between systems.
* **Logging System**: console and file logging, with an optional verbose mode.

The architecture ensures that no system depends on implementation details of another system; only the ECS and event bus are shared.

---

## 3. Entity–Component–System Design

### 3.1 Entities

Entities are numeric identifiers (32-bit). They have no inherent data; all data is stored in components.

### 3.2 Components

At MVP stage, the following components are required:

* **TransformComponent**: position and orientation.
* **VelocityComponent**: speed and direction.
* **HitboxComponent**: collision shape and size.
* **HealthComponent**: hit points for destroyable entities.
* **PlayerInputComponent**: latest validated input state.
* **MonsterAIComponent**: pattern type (for current monster).
* **MissileComponent**: projectile ownership and damage.

Additional components will be added in later iterations.

### 3.3 Systems

Each system operates only on entities containing the required components.

* **MovementSystem**: integrates position using velocity and delta-time.
* **CollisionSystem**: detects collisions using AABB (Axis-Aligned Bounding Boxes).
* **MonsterSystem**: applies movement and shooting behaviors according to the assigned pattern.
* **PlayerSystem**: processes player actions based on inputs received.
* **MissileSystem**: moves missiles and checks for destruction conditions.
* **SpawnSystem**: generates monster entities at appropriate intervals.
* **DestructionSystem**: handles entity removal after health reaches zero.

---

## 4. Networking Model

### 4.1 Protocol Requirements

The server uses:

* **UDP** for all in-game communication.
* **Binary packets** with a clearly defined header and payload.
* **Robust handling of malformed data** to ensure no crashes.
* **A delta-state update model** to minimize bandwidth consumption.

### 4.2 Packet Structure

All packets follow a common format:

```
[Header]
  uint8_t messageType
  uint16_t sequenceId
  uint32_t tickId

[Payload]
  message-specific binary structure
```

The header provides message classification, ordering, and association with the current server tick.

### 4.3 Message Ordering

The server assigns both:

* a **sequence ID per client**, incremented at each received packet.
* a **global tick ID**, used in outbound packets.

This ensures reliable ordering and allows clients to discard outdated packets.

### 4.4 Supported Message Types (MVP)

Incoming from clients:

* **INPUT_STATE**

Outgoing from server:

* **DELTA_UPDATE**
* **ENTITY_SPAWN**
* **ENTITY_DESTROYED**
* **PLAYER_DISCONNECTED**

The protocol documentation will detail each binary format.

---

## 5. Game Loop

### 5.1 Tick Rate

The server runs at a fixed rate of **60 updates per second**.

### 5.2 Execution Order per Tick

1. Read and queue all UDP packets received since last update.
2. Apply validated player inputs.
3. Run Gameplay Systems:

   * PlayerSystem
   * MonsterSystem
   * MissileSystem
   * SpawnSystem
4. Run MovementSystem.
5. Run CollisionSystem.
6. Run DestructionSystem.
7. Build and send delta-state updates to all clients.

### 5.3 Delta-State Logic

A delta-state update transmits only:

* entities that moved
* entities that spawned since the last update
* entities that died

This approach minimizes bandwidth usage compared to full snapshots.

---

## 6. Multithreading Model

### 6.1 Thread Roles

The server uses at minimum:

* **Game Thread**: runs the game loop at fixed intervals.
* **Network Thread**: handles UDP reception, parsing, and forwarding input events.

### 6.2 Thread Communication

A thread-safe queue is used for:

* pushing received input states to the game thread
* pushing outgoing packets from the game logic to the networking thread

This ensures the game loop never blocks on network operations.

---

## 7. Monster Behavior

### 7.1 MVP Patterns

Two to three initial patterns are supported:

* Linear movement from right to left.
* Sinusoidal horizontal movement.
* Zigzag vertical movement.

These are stored in MonsterAIComponent.

### 7.2 Future Extendability

The ECS design allows adding new patterns without modifying existing system logic; only new pattern handlers must be implemented.

---

## 8. Collision Detection

### 8.1 Collision Shape

AABB (Axis-Aligned Bounding Boxes) is used for its simplicity and speed.
Each entity declares:

* width
* height
* offset relative to its transform

### 8.2 Collision Resolution

The collision system identifies:

* player hit by monster missile
* monster hit by player missile
* player colliding with monster

When a collision is detected:

* A damage event is emitted.
* The DestructionSystem applies consequences.

---

## 9. ID System

All entities and players use 32-bit numeric IDs. This avoids limits during future extensions.

Player IDs are unique per connection. Entity IDs are unique per game instance.

---

## 10. Logging

### 10.1 Default Logging

* Console logs minimal events: player join/leave, errors, critical system messages.

### 10.2 File Logging

* Errors and warnings are persisted in a rotating log file.

### 10.3 Verbose Mode

A command-line flag enables verbose logging, printing internal state changes and network operations.

---

## 11. Error Handling and Robustness

The server never crashes due to:

* malformed packets
* oversized payloads
* missing components
* unexpected disconnects

Invalid packets are ignored and optionally logged.

If a client disconnects, a PLAYER_DISCONNECTED message is broadcast.

---

## 12. Future Extensions

The architecture supports:

* additional monster types
* complex AI patterns
* multiple game instances
* scripting
* advanced physics
* content pipeline tools

The modular ECS and subsystem structure allows significant growth without redesigning the core.

---

## 13. Conclusion

This document defines the complete server architecture for the R-Type game. All design choices respect the project specification and aim to provide a robust, scalable, and extendable foundation. The server is authoritative, deterministic, networked via binary UDP, and supports future gameplay and technical extensions.

Further documentation will include:

* complete protocol specification
* client architecture
* build and packaging instructions
* developer onboarding and contribution guidelines
