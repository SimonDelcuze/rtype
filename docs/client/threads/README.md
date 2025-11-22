# Threads

The client uses a multithreaded networking architecture to ensure responsive input handling and smooth state synchronization with the server.\
Unlike the server, the client’s threads do not execute gameplay logic and do not access the ECS directly. Their purpose is to manage network communication without blocking the rendering or prediction pipeline.

This page explains the overall threading model used by the client, before detailing each individual thread.

***

### **1. Why the Client Needs Multiple Threads**

The client must perform several tasks simultaneously:

* read user input
* render frames at a high and variable frame rate
* send input to the server at a stable frequency
* receive server snapshots at unpredictable intervals

If all these tasks were performed on a single thread, the client would suffer from:

* input delay when waiting for network operations
* dropped frames when processing packets
* jitter in interpolation
* visible stalls during high-latency periods

To avoid this, the networking workload is separated into dedicated threads.

***

### **2. Thread Responsibilities**

The client uses two networking threads:

#### **1. Receive Thread**

This thread is responsible for handling all incoming UDP packets from the server.\
Its duties include:

* reading server snapshots
* validating packets
* writing snapshot data into a safe buffer
* handling entity creation and destruction updates
* maintaining ordering via sequence IDs

This thread never interacts with the ECS.\
It only stores data for later consumption by the main thread.

***

#### **2. Send Thread**

This thread sends user input to the server at a fixed rate.\
Its responsibilities include:

* reading local input state
* encoding it into the client input packet format
* attaching a sequence ID for ordering
* sending the packet via UDP
* storing the input in the prediction history buffer

Just like the receive thread, the send thread does not modify the ECS.

***

### **3. Main Thread vs. Networking Threads**

The **main thread** runs the client’s actual game loop:

* prediction
* interpolation
* snapshot application
* animation
* rendering
* UI
* audio

The **networking threads** operate fully independently.

Communication between them happens through:

* thread-safe message queues (snapshots)
* atomic variables
* double-buffered state objects

The key design rule is:

**Only the main thread modifies the ECS.**\
This prevents race conditions and ensures stable rendering and predictable simulation.

***

### **4. Synchronization Model**

The client’s network threads and main thread use a strictly controlled synchronization pattern:

* Receive Thread writes snapshots into a buffer
* Main Thread reads them during the update phase
* Send Thread writes input history entries
* Prediction and Reconciliation read from this history

There are no locks on the ECS registry and no blocking operations in the main loop.

The result is a responsive, jitter-free client even under poor network conditions.

***

### **5. Failure and Latency Handling**

Because network conditions are unpredictable, the architecture is designed to remain stable even when:

* snapshots arrive late
* inputs are temporarily unsent
* packets are lost or duplicated
* the server lags or spikes
* the connection drops temporarily

The multithreaded design isolates these failures from the rendering and prediction subsystems.\
The client continues to run, animate, and interpolate smoothly.
