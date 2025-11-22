# Receive Thread

The Receive Thread is responsible for handling all incoming UDP packets from clients.\
Because UDP is asynchronous, unordered, and unreliable, this thread isolates all untrusted network input from the rest of the server.\
Its objective is to safely convert raw datagrams into validated input commands that the game loop can consume.

***

### **1. Responsibilities**

The Receive Thread performs the following tasks:

* Listening for UDP datagrams from clients.
* Validating packet structure, size, and message type.
* Ensuring sequence ordering through per-client sequence IDs.
* Parsing input data (movement, aiming, firing, etc.).
* Rejecting malformed or suspicious packets.
* Pushing validated input events into a thread-safe queue for the main game loop.

This thread never interacts directly with the ECS registry or gameplay systems.\
Its only responsibility is to prepare sanitized client input for the main loop.

***

### **2. Data Flow**

The Receive Thread runs independently from the main loop. When a packet arrives, it performs the following steps:

1. Receive a datagram from the UDP socket.
2. Verify that the packet length matches the expected size for an input message.
3. Decode and validate the packet header.
4. Extract the sequence ID and ensure it is strictly increasing.
5. Decode the input structure.
6. Clamp or discard values that exceed allowed ranges.
7. Push the resulting input command into a thread-safe input queue.
8. Return immediately to wait for the next packet.

The input queue is drained once per simulation tick by the Game Loop Thread.

***

### **3. Packet Validation**

Incoming packets undergo several validation layers:

#### Size Validation

Packets with size smaller or larger than the expected format are discarded immediately.

#### Message Type Validation

The header must indicate a known message type; unknown types are ignored.

#### Sequence ID Validation

Each client maintains a monotonically increasing `sequenceId`.\
If a packet arrives out of order or with a stale ID, it is ignored.\
This prevents reordering issues and ensures deterministic input application.

#### Data Sanity Validation

The thread imposes clamping rules on fields such as movement direction and aim angle to prevent corrupted values:

* Movement vectors must be within the normalized range.
* Angles must be finite, non-NaN values.
* Boolean flags must be valid (0 or 1).

Malformed packets are dropped without affecting the server.

***

### **4. Input Decoding**

After validation, the packet is decoded into a `PlayerInput` structure.\
A typical decoded input contains:

* Movement axes (dx, dy)
* Aim angle
* Shooting flag
* Sequence ID
* Client identifier (added by the server)

The Receive Thread does not modify gameplay state and does not apply the input.\
It only stores the input for later consumption.

***

### **5. Thread-Safe Queue**

Since this thread runs concurrently with the simulation thread, input cannot be applied immediately.\
Instead, validated inputs are inserted into a thread-safe queue.\
The queue may be implemented using:

* A mutex-protected container
* A lock-free structure
* A multiple-producer, single-consumer model

During each simulation tick, the Game Loop Thread drains the queue and applies all pending inputs for that tick.

This guarantees deterministic processing and ensures that no network operation interrupts the main simulation.

***

### **6. Error and Failure Handling**

The Receive Thread is designed to continue functioning under adverse conditions.\
It handles:

* Unknown clients
* Clients sending malformed packets
* Packet floods (rate limiting may be used)
* Lost or duplicated packets
* Clients disconnecting abruptly

In all these cases, the thread continues to operate without stopping the simulation.

Invalid data never reaches the ECS world.

***

### **7. Purpose and Design Rationale**

The Receive Thread exists to:

* Prevent the game loop from blocking on network I/O.
* Protect gameplay logic from untrusted network data.
* Maintain strict input ordering.
* Provide deterministic tick-by-tick input consumption.
* Improve responsiveness by handling packets as soon as they arrive.

Separating networking from simulation is essential for stable real-time multiplayer behavior.
