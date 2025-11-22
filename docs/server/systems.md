# Systems

The server uses a collection of gameplay-focused ECS systems that together define the authoritative game simulation.\
These systems implement all gameplay logic and operate exclusively within the server’s Game Loop thread, ensuring deterministic and consistent updates for all connected clients.

Server systems read and modify ECS components to execute tasks such as:

* processing player inputs
* updating entity movement
* running AI behaviors
* detecting and resolving collisions
* applying damage
* spawning or destroying entities
* preparing delta-state snapshots for networking

Each system operates on a specific subset of components, and systems are executed in a controlled, deterministic order to maintain simulation stability.

This section documents every system used by the authoritative server.\
Each system will have its own subpage describing:

* its purpose
* the components it reads and writes
* its role within the simulation pipeline
* the assumptions it makes about component states
* how it contributes to the delta-state and synchronization process

Systems will be added and documented progressively as they are introduced into the server architecture.
