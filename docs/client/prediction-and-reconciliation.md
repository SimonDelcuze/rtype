# Prediction & Reconciliation

Prediction and reconciliation are essential techniques that allow the client to remain responsive while maintaining full consistency with the authoritative server state.\
Because UDP introduces latency, packet loss, and irregular delivery, the client cannot wait for the server before displaying movement or reactions. Instead, it must predict locally while still correcting errors when authoritative data arrives.

This page describes how prediction and reconciliation work on the client side and how they integrate into the main game loop.

***

### **1. Purpose of Prediction**

Prediction is used exclusively for the local player to avoid visible input delay.

If the client waited for the server to approve each movement, the game would feel unresponsive.\
To avoid this, the client:

* reads user input
* applies it immediately
* updates the local player’s ECS components
* sends the input to the server with a sequence ID
* stores the input in a local history buffer

This allows instantaneous movement, regardless of network latency.

Remote entities never use prediction.

***

### **2. Prediction Workflow**

During each frame of the main game loop:

1. The player’s current input (movement, shooting, aiming) is captured.
2. A local velocity or movement response is computed.
3. The client applies the corresponding ECS updates immediately.
4. The input is assigned a sequence ID and sent to the server.
5. A copy of the input is pushed into the prediction history buffer.

Prediction only ensures responsiveness; it does not replace authoritative data.

***

### **3. Server Authority and Snapshots**

The server runs the actual game simulation.\
It sends snapshots containing the authoritative positions of all entities, including the local player.

When a snapshot arrives:

* It contains a sequence ID indicating the last input processed by the server.
* It contains the authoritative position of the player.

The client must compare this authoritative state with its predicted state to detect inconsistencies.

***

### **4. Detecting Divergence**

A divergence occurs when:

* the predicted local position differently from the server-provided position
* accumulated prediction error becomes visible
* inputs were predicted incorrectly (due to physics, collisions, AI interactions, etc.)

The client detects divergence by comparing:

* predicted position from ECS
* authoritative position from the snapshot
* sequence ID of last processed input

If the discrepancy is greater than a small tolerance threshold, reconciliation is performed.

***

### **5. Reconciliation Process**

The reconciliation process restores consistency while preserving responsiveness.\
When divergence is detected:

1. The client overwrites the local player’s transform with the authoritative position from the snapshot.
2. It then looks at the prediction history buffer to find all inputs the server has not yet processed.
3. These unconfirmed inputs are replayed on top of the authoritative state.
4. The corrected position becomes the new predicted state.

The result is a smooth correction that respects both the server’s logic and the player’s expectations.

***

### **6. Input History Buffer**

The input history buffer stores each input sent to the server.\
For each input, the client saves:

* movement axes
* aiming direction
* shooting flags
* the associated sequence ID
* timestamp or frame index

When a snapshot arrives:

* the client identifies which inputs the server has already processed
* all older inputs are discarded
* only unconfirmed inputs are replayed

This process is essential for accurate reconciliation.

***

### **7. Dealing with Latency and Packet Loss**

Prediction is designed to handle various network issues:

* **High latency**\
  More unconfirmed inputs accumulate, but correction still works.
* **Packet loss**\
  Missed snapshots are replaced by future ones; reconciliation remains valid.
* **Out-of-order packets**\
  The sequence ID ensures only the newest authoritative state is applied.
* **Intermittent freeze or spike**\
  Prediction continues smoothly; reconciliation corrects the state afterward.

The client remains visually stable and responsive as long as snapshots eventually arrive.

***

### **8. Limitations**

Prediction and reconciliation cannot solve all network artifacts. Potential limitations include:

* visible correction if prediction error becomes large
* abrupt movement adjustments after long packet loss
* noticeable snapping if the server’s authoritative logic diverges significantly

These behaviors are minimized through:

* tight integration with the ECS
* deterministic server simulation
* careful design of collision and movement systems

***

### **9. Summary**

Prediction and reconciliation ensure:

* instant responsiveness to player input
* full consistency with the authoritative server state
* smooth corrections even under unstable network conditions
* separation of local smoothing and authoritative logic

This subsystem is fundamental to delivering a responsive multiplayer experience without sacrificing correctness.
