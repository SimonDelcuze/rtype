# Receive Thread

The Receive Thread is responsible for handling all incoming UDP packets from the server.\
Its design isolates untrusted network input from the rest of the client, ensuring that rendering, prediction, and local ECS updates remain stable even if network conditions degrade.

This thread never interacts directly with the ECS.\
It simply collects snapshots and stores them in a safe buffer for the main thread to process during the next update cycle.

***

### **1. Responsibilities**

The Receive Thread performs the following tasks:

* Listening for incoming UDP datagrams from the server
* Validating the structure of received packets
* Checking sequence IDs and discarding outdated snapshots
* Decoding authoritative state updates
* Storing the decoded snapshot in a thread-safe buffer
* Notifying the main thread that new data is available

Snapshots are not immediately applied to the ECS; they are only queued for consumption by the main thread.

***

### **2. Packet Validation**

Incoming packets must satisfy several conditions before being accepted:

**Size validation**\
Packets must match the expected structure of a server snapshot. Unexpected sizes are discarded.

**Message type validation**\
Only valid snapshot packets are processed. Unknown packet types are ignored.

**Sequence ID validation**\
Each snapshot includes a monotonic sequence ID.\
Snapshots arriving out-of-order or duplicated are skipped to maintain consistency.

**Sanity checks**\
Basic checks ensure that the decoded data is valid and does not contain:

* non-finite numerical values
* invalid entity identifiers
* corrupted component data

Invalid packets never reach the rendering or prediction systems.

***

### **3. Snapshot Buffering**

Because snapshots arrive asynchronously, the Receive Thread cannot modify the ECS.\
Instead, it writes decoded snapshots to a dedicated buffer.\
A typical buffering strategy includes:

* a double-buffer system, or
* a thread-safe queue that stores a limited number of recent snapshots.

The main thread reads from this buffer once per update and applies the most recent snapshot to the ECS through the replication layer.

This design avoids blocking the rendering or prediction pipeline and prevents race conditions.

***

### **4. Interaction with Prediction and Interpolation**

The Receive Thread does not perform prediction or interpolation itself, but the data it provides is essential for both:

* Prediction uses snapshots to perform reconciliation for the local player.
* Interpolation uses snapshots to update the target positions of remote entities.

The receive thread’s job is not to compute these; it only ensures that all required data is available and valid.

***

### **5. Error Handling and Robustness**

The Receive Thread must remain operational even under adverse network conditions.\
It handles:

* packet loss
* packet duplication
* packet reordering
* latency spikes
* corrupted packets
* unexpected disconnects

None of these conditions crash the client or block the main thread.\
New snapshots will simply overwrite old ones, and the main thread will interpolate or correct as needed.

### **6. Summary**

The Receive Thread ensures that:

* networking does not interrupt rendering or prediction
* snapshots are correctly parsed and validated
* the ECS is updated only by the main thread
* the client remains stable under all network conditions

Its role is strictly limited to receiving, validating, and buffering authoritative state updates from the server.
