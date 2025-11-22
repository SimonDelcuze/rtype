# Threads

The server uses a multithreaded architecture to guarantee deterministic gameplay simulation and responsive networking under all conditions.\
Threads are strictly separated by responsibility and communicate only through controlled data structures, ensuring high stability and preventing race conditions inside the ECS world.

This page provides a global explanation of why the server uses multiple threads, how they interact, and what constraints they must respect. Individual thread behaviors are documented in the following pages.

***

### **1. Purpose of a Multithreaded Server**

Real-time multiplayer games must perform several tasks simultaneously:

* Listen for network packets from multiple clients
* Process and validate incoming data
* Simulate the entire game world at a fixed rate
* Serialize and send updates back to clients
* Manage connection states
* Handle spawn logic, timing, and cleanup

If these tasks ran on a single thread, the server would become unstable under load, and gameplay timing would fluctuate based on network conditions.

The multithreaded design isolates responsibilities and ensures the simulation remains consistent regardless of external factors such as network delay or variable client performance.

***

### **2. Thread Separation and Responsibilities**

The server runs three main threads:

#### **1. Receive Thread**

Dedicated to handling all incoming UDP traffic.\
This thread validates packets and pushes sanitized input into a thread-safe queue.

#### **2. Game Loop Thread**

The authoritative simulation thread.\
Runs the ECS pipeline at a fixed 60 Hz tick rate, processes inputs, updates entities, and produces the delta-state snapshot.

#### **3. Send Thread**

Responsible for serializing and sending the latest snapshot to all clients.\
This ensures network output never blocks the simulation.

This separation ensures that networking cannot interrupt or slow down the gameplay logic.

***

### **3. Communication Between Threads**

Threads never access the ECS registry concurrently.\
Instead, they communicate through safe, controlled channels:

* The Receive Thread writes inputs into an input queue.
* The Game Loop Thread drains this input queue each tick.
* The Game Loop writes the snapshot into a protected buffer.
* The Send Thread reads this snapshot buffer and transmits the data.

No direct shared access to gameplay state occurs between threads.

This design avoids:

* race conditions
* inconsistent entity updates
* corrupted ECS state
* blocking operations inside the simulation loop

***

### **4. Determinism Requirements**

The Game Loop Thread operates with strict timing requirements.\
To guarantee a stable and authoritative simulation, it must:

* avoid blocking operations
* avoid waiting for network I/O
* run at a fixed update rate
* apply all inputs in deterministic order

This is why threads are heavily isolated, and synchronization points are minimized.

The Receive and Send Threads operate opportunistically and asynchronously, while the Game Loop Thread advances the world with predictable timing regardless of network performance.

***

### **5. Constraints and Safety Rules**

The server enforces several design constraints to maintain stability:

* Only the Game Loop Thread may modify the ECS registry or gameplay state.
* Threads may not call into gameplay systems outside the simulation step.
* Shared memory access is limited to explicitly synchronized structures.
* All network input is sanitized before reaching the ECS.
* Outgoing packets are produced only from finalized snapshots.
* The server must continue running even if a client behaves incorrectly.

These constraints ensure a robust server that behaves consistently under heavy load, packet loss, or malformed client messages.

***

### **6. Advantages of This Architecture**

This threading model provides several benefits:

* The simulation loop remains stable and immune to network variability.
* Packet processing happens as soon as data arrives, without delaying gameplay.
* Outgoing packets are sent reliably without stalling the game loop.
* ECS state is never accessed concurrently, eliminating race conditions.
* The server remains responsive even when clients are slow or unstable.
* The design scales cleanly as more entities or players are added.

***

### **7. Next Pages**

The next pages describe each thread in detail:

* Receive Thread
* Game Loop Thread
* Send Thread

Each will explain its internal workflow, data flow, error handling strategy, and interaction with the rest of the server.
