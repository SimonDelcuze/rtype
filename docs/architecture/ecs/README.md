# ECS Architecture

Our engine is built around a fully custom Entity–Component–System (ECS) architecture, designed for:

* high performance
* low memory fragmentation
* deterministic server-side simulation
* decoupled client/server logic
* network-friendly data structures
* simple extensibility

This page explains the global structure of the ECS framework used across the entire project.\
More detailed system- and component-specific information appears later in the SERVER and CLIENT sections.

***

### **1. What Is ECS?**

The ECS design pattern separates game functionality into three independent layers:

#### **Entities**

* Simply integer identifiers (e.g., `uint32_t`).
* Represent any element of the game world: players, missiles, monsters, backgrounds, etc.
* Contain no data by themselves.

#### **Components**

* Pure data structures (no logic).
* Attached to entities to define their state.
* Stored in dense, cache-friendly contiguous arrays for fast iteration.
* Examples include:
  * `TransformComponent`
  * `VelocityComponent`
  * `HitboxComponent`
  * `HealthComponent`
  * `SpriteComponent` (client-only)

#### **Systems**

* Contain all logic and behavior.
* Operate only on entities that possess specific components.
* Executed in deterministic order inside the game loop (server) or render loop (client).
* Examples include:
  * `MovementSystem`
  * `CollisionSystem`
  * `MonsterAISystem`
  * `AnimationSystem` (client)

***

### **2. Diagram – ECS Architecture Overview**

<figure><img src="../../.gitbook/assets/ecs_EN (2).png" alt=""><figcaption></figcaption></figure>

***

### **3. ECS Goals and Design Principles**

Our ECS framework was designed with the following core objectives:

#### **Decoupling**

* Systems do not communicate directly.
* They interact exclusively through reading and writing components.
* This keeps logic clean, local, and easy to extend.

#### **Network Compatibility**

* Authoritative ECS state lives on the server.
* Only the relevant components are serialized into delta-snapshots.
* Client ECS replicas store only visual or predicted data.

#### **Performance**

Dense/sparse arrays provide:

* O(1) component lookup
* contiguous memory for cache-friendly iteration
* fast processing over active entities

This is crucial for:

* server simulation performance
* smooth client rendering
* high entity counts

#### **Determinism (Server-Side)**

The server runs:

* a fixed timestep (60 FPS)
* strictly ordered ECS system execution
* pure logic with no randomness unless explicitly seeded

This guarantees consistent multiplayer simulation.

#### **Extensibility**

New components and systems can be added without changes to existing ones.\
This is achieved through:

* type IDs generated at runtime
* template-driven registries
* simple system integration
* clean separation between data and behavior

***

### **4. Shared vs Client vs Server ECS**

Although the same ECS framework is used across the project, each layer uses it differently:

#### **Shared ECS**

* Contains the core registry implementation
* Includes low-level utilities: type IDs, views, sparse sets
* Contains universal components like `MovementComponent`

#### **Server ECS**

* Contains only gameplay components
* Runs all authoritative gameplay systems
* Never includes rendering or animation systems
* Generates snapshots

#### **Client ECS**

* Contains visual and predicted components
* Uses systems for animation, interpolation, rendering
* Synchronizes authoritative server components
* Never contains real gameplay systems

This separation is essential for multiplayer correctness and performance.
