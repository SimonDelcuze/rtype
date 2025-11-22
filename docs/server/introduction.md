# Introduction

The **Server** is the authoritative core of the entire R-Type game.\
It runs the full simulation, enforces the rules, validates client actions, and ensures that every player shares the same game state.

In our architecture, the server is:

* **Authoritative** → the single source of truth
* **Deterministic** → fixed 60 FPS simulation loop
* **Multithreaded** → networking threads + game loop thread
* **ECS-based** → all world state encoded in components
* **Robust** → designed to continue running even if clients disconnect or send invalid data
* **Stateless toward clients** → clients cannot influence game logic except through validated input

This section explains the entire server architecture, its threading model, ECS usage, systems, and the full delta-state networking pipeline.

***

## **1. Role of the Server**

The server’s mission is to simulate the game world independently from the clients.

It is responsible for:

#### Validating inputs

Clients may lie, lag, or behave inconsistently.\
The server ensures inputs are:

* ordered
* filtered
* safe
* applied deterministically

#### Running the entire gameplay simulation

The server executes:

* Player movement
* Monster AI
* Missile behavior
* Collisions
* Damage calculation
* Entity destruction and spawning

#### Managing all entities and components

The server hosts the **authoritative ECS registry**, containing:

* All components important for gameplay
* All entities currently in the game
* Systems performing logic

#### Sending game state updates to clients

Clients do **not** simulate the world fully.\
They only perform prediction for smoothness.

The server sends:

* entity spawns
* delta-state updates
* destructions
* player updates
* monster AI actions
* missile creation

#### ✔ Remaining stable under failures

Even if clients disconnect, send malformed packets, or lag, the server continues running safely.

***

## **2. Threading Model**

The server uses a multithreaded architecture to avoid blocking the simulation loop.

It runs:

#### **1. UDP Receive Thread**

* Listens for client packets
* Validates packet structure
* Extracts input commands
* Pushes them into a thread-safe queue

#### **2. Game Loop Thread (60 FPS)**

Runs the entire ECS simulation:

1. Read all queued inputs
2. Apply PlayerInputSystem
3. Update monsters (MonsterAISystem)
4. Simulate movement
5. Run collision resolution
6. Apply damage
7. Destroy dead entities
8. Spawn new enemies
9. Build delta-state snapshots

#### **3. UDP Send Thread**

* Reads the latest delta-state
* Sends compact binary updates to each connected client
* Avoids blocking the main loop

This ensures the server never waits on network operations.

***

## **3. Server ECS**

The server’s ECS contains **only gameplay components**—no rendering, no animation, no textures.

Examples include:

* Transform
* Velocity
* Hitbox
* Health
* MonsterAI
* Missile
* PlayerInput
* Lifetime timers

(Each of these will be documented later in the _Components_ subpage.)

Systems handle all gameplay logic in a deterministic pipeline.

***

## **4. Networking Philosophy**

The server uses:

* **UDP** for all gameplay communication
* A **binary protocol**
* Fixed-size and variable sections for optimal bandwidth
* Sequence IDs to ensure ordering
* Delta-state to reduce packet size
* Stateless per-client communication

Clients never send:

* entity spawns
* transformations
* health values
* collision events

They send only **input commands**.

Everything else comes from the server.

***

## **5. Why a Server-Authoritative Model?**

This design ensures:

#### Synchronization

All players see the same world, not their own versions.

#### Anti-cheat

Clients can't modify core state.

#### Stability

Even a buggy client can’t corrupt the game.

#### Interpolation-friendly snapshots

Clients receive just enough information to:

* interpolate
* predict
* correct

without running full logic.

***

## **6. What Comes Next**

In the next subpages, we will explore:

#### **Server → Architecture Overview**

Full breakdown of the server pipeline with diagrams.

#### **Server → Threads**

Detailed explanation of each thread:

* Receive thread
* Main loop
* Send thread

#### **Server → Components**

All server-side components, their purpose, and their data structure.

#### **Server → Systems**

Full explanation of server systems and their execution order.

#### **Server → Delta-State Pipeline**

How snapshots are built, compressed, and sent efficiently.

***

##
