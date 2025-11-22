# Send Thread

The Send Thread is responsible for transmitting the player's input to the server at a stable and predictable rate.\
This thread operates independently from the rendering and prediction systems to ensure that network communication does not introduce latency or delays into the main game loop.

The Send Thread performs no gameplay logic and does not interact with the ECS directly.\
Its sole purpose is to package user input into a network message, attach the necessary metadata, and send it to the server via UDP.

***

### **1. Responsibilities**

The Send Thread handles the following operations:

* Reading the player’s current input state from the main thread
* Encoding the input into the client’s input packet format
* Attaching a monotonically increasing sequence ID
* Sending the packet through the UDP socket
* Storing the input in a local history buffer for prediction reconciliation
* Ensuring that inputs are sent at a steady frequency, independent of frame rate

This prevents rendering fluctuations or momentary frame drops from affecting input transmission.

***

### **2. Input Collection**

The Send Thread does not read input devices directly.\
Instead, the main thread provides the current input state (movement, aiming, shooting) through a safe shared structure.

This ensures:

* no race conditions between input handling and prediction
* no direct dependency on the main loop’s timing
* complete decoupling between rendering and networking

The Send Thread simply reads the latest input snapshot available.

***

### **3. Packet Construction**

For each iteration of its update loop, the Send Thread constructs a client input packet containing:

* movement axes or directional vector
* aiming angle
* firing or action flags
* a sequence ID
* optional timestamp data
* optional local player identifier

The packet format is consistent with the server’s expected message layout.\
Sequence IDs ensure proper ordering and allow the server to discard outdated or duplicated inputs.

***

### **4. Sequence ID Management**

Each input packet includes a sequence ID that increments with every input sent.\
This provides several benefits:

* the server can maintain strict input ordering
* the server can detect and ignore stale packets
* the client’s prediction system can reconcile authoritative corrections

The Send Thread is the only part of the client responsible for incrementing and attaching this sequence ID.

***

### **5. Prediction History Buffer**

After sending each input, the thread stores it in a local history buffer.\
This buffer is used by the main thread during reconciliation:

* when a snapshot arrives, the client checks which inputs the server has already processed
* remaining unconfirmed inputs are reapplied for prediction
* mismatches between predicted and authoritative positions are corrected

The Send Thread only stores the history; the main thread handles the prediction logic.

***

### **6. Transmission Frequency**

The Send Thread runs at a fixed frequency, usually matching the server’s tick rate (commonly 60 Hz).\
This stable interval guarantees:

* consistent input flow
* predictable server behavior
* reduced network jitter
* minimized packet bursts

The thread must not sleep excessively or adjust frequency based on framerate.

***

### **7. Error Handling and Robustness**

The Send Thread is designed to remain functional even when:

* packet loss occurs
* the server fails to acknowledge inputs
* network latency fluctuates
* socket operations momentarily fail
* the user experiences frame drops

Lost or delayed input packets are not resent by the client; they are simply replaced by newer packets.\
The authoritative server and reconciliation system ensure synchronization.

***

### **8. Summary**

The Send Thread ensures that:

* inputs are transmitted consistently and efficiently
* the client maintains responsive controls even under poor network conditions
* prediction and reconciliation remain accurate
* networking operations do not interfere with rendering or ECS processing

Its architecture isolates input transmission from the rest of the client to guarantee smooth and predictable gameplay.
