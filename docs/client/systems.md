# Systems

The client uses a set of ECS systems dedicated to visual processing, prediction, state smoothing, and frame construction.\
Unlike the server, the client does not execute authoritative gameplay logic.\
Instead, its systems operate on locally stored data to provide a responsive and visually consistent representation of the authoritative game world.

Each system is responsible for a specific step of the client-side pipeline, such as:

* applying authoritative snapshots
* performing local prediction and reconciliation
* interpolating remote entities
* updating animations
* rendering the final frame

Client systems do not modify gameplay simulation rules.\
They interpret, smooth, and display the data produced by the server.

This section documents every system used by the client.\
Each system will have its own subpage describing:

* its purpose
* the data it reads and writes
* how it fits into the client-side pipeline
* how it interacts with snapshots, prediction, and interpolation
* how it contributes to the rendering flow

Systems will be added progressively as they are introduced into the client architecture.

System list:
