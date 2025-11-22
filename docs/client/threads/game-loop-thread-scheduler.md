# Game Loop Thread (Scheduler)

The client’s game loop is responsible for coordinating all local simulation tasks, including prediction, reconciliation, interpolation, animation updates, and rendering.\
Unlike the server, the client does not execute authoritative gameplay logic. Its loop is focused on producing a smooth visual experience while incorporating the server’s authoritative state safely and consistently.

The game loop runs on the main thread and must remain stable at high frame rates, independent of network performance.

***

### **1. Responsibilities of the Client Game Loop**

During each iteration, the client game loop performs the following tasks:

1. Read the current input state
2. Apply prediction for the local player
3. Process new snapshots received from the server
4. Reconcile predicted and authoritative positions
5. Interpolate remote entities to smooth motion
6. Update animations
7. Render the scene
8. Update UI and audio

All ECS modifications occur in this thread.\
Networking threads only provide data through thread-safe buffers.

***

### **2. Input Handling**

The first step is to read raw user input (keyboard, mouse, controller).\
The game loop:

* captures movement axes
* reads shooting or action buttons
* computes the aiming direction
* writes the result to a shared structure for the Send Thread

This ensures the Send Thread always has the most recent input snapshot without blocking the main loop.

***

### **3. Local Prediction**

To avoid input delay, the client immediately applies the player's input to the local ECS:

* velocity updates
* position changes
* aiming adjustments

Prediction makes the game feel responsive even if the server snapshot arrives late.\
Each predicted input is stored with its sequence ID for later reconciliation.

***

### **4. Snapshot Processing (Replication Layer)**

If the Receive Thread has produced a new snapshot, the Game Loop retrieves it.\
The snapshot contains:

* updated positions
* health changes
* entity creation events
* entity destruction events

The replication layer updates the ECS accordingly.

The local player’s position is not simply overwritten; it is handled through reconciliation.

***

### **5. Reconciliation**

Reconciliation compares:

* the predicted local position
* the authoritative position from the snapshot

If the two differ, the client:

1. corrects the position to match the snapshot
2. replays all unconfirmed inputs since the snapshot’s sequence ID

This ensures visual consistency while maintaining responsive controls.

Remote entities are not predicted and therefore do not undergo reconciliation.

***

### **6. Interpolation of Remote Entities**

Remote players, monsters, and other entities use interpolation rather than prediction.\
Interpolation prevents jitter caused by:

* irregular snapshot arrival
* UDP packet loss
* variation between client framerate and server tick rate

For each remote entity:

* the previous and current server positions are stored
* the Game Loop computes an interpolation factor
* the entity is placed between these positions for rendering

This produces smooth movement even when network traffic is unstable.

***

### **7. Animation Update**

The Game Loop updates visual animations based on:

* elapsed frame time
* current animation state
* animation component fields
* loop and frame timing rules

Each entity with an animation component advances its frame according to its animation definition.

***

### **8. Rendering**

Finally, the Game Loop:

* computes final render transforms
* applies sprite frames
* determines draw order
* renders all visible entities
* draws the UI and HUD

Rendering is the last stage of the loop.\
Only components required for visuals are considered.

***

### **9. Timing Model**

Unlike the server, which uses a fixed 60 Hz tick, the client rendering loop runs:

* at the monitor refresh rate
* or at a capped rate chosen by the application

Interpolation and prediction handle any mismatch between rendering and server tick rates.

The client game loop must never block on network operations or long computations.

***

### **10. Summary**

The client’s game loop is responsible for:

* responsive controls through prediction
* consistency through reconciliation
* smooth visuals through interpolation
* correct representation of the world through snapshot application
* all rendering and animation updates

It acts as the central coordinator of all client-side subsystems while maintaining full isolation from network threads.
