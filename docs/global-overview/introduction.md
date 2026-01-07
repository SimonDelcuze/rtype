# Introduction

The Global Overview section provides a high-level understanding of how the different parts of the engine interact:\
server simulation, client rendering, networking, and the shared ECS architecture.

This section is the recommended starting point before diving into the server or client pages.\
It explains:

* the core principles of the project
* how responsibilities are divided between the server and the client
* what role the ECS plays in both sides
* how networking ties everything together
* how the multi-instance architecture enables concurrent game sessions
* how the different threads and subsystems communicate
* how snapshots, prediction, and interpolation work

You will also find a complete architecture diagram summarizing the entire workflow of the engine.

After reading this section, you will have a clear mental model of:

* the overall data flow
* the purpose of each subsystem
* where each piece of logic lives
* how the engine maintains determinism, performance, and visual smoothness

From here, you can continue to:

* **Multi-Instance Architecture** to understand how the server supports multiple concurrent games
* **Server Architecture** to understand authoritative simulation
* **Client Architecture** to see how visuals and networking are handled
* **ECS Overview** to learn how entities, components, and systems are implemented

The following pages break down each major part of the engine step by step.
