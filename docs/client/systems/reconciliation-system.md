# Reconciliation System

The Reconciliation System implements client-side prediction with server reconciliation to provide smooth, responsive gameplay while maintaining server authority.

## Problem

In online multiplayer games, there's always network latency between the client and server. Without client-side prediction:
- Player movement feels laggy (inputs take RTT/2 to show effect)
- Game feels unresponsive and difficult to control

With naive client-side prediction:
- Client predicts movement immediately (feels responsive)
- But when server state arrives, position "snaps" to correct location
- Creates jarring visual artifacts

## Solution: Server Reconciliation

The reconciliation system solves this by:
1. **Predicting** - Client applies inputs immediately for responsive feel
2. **Storing** - Client keeps history of unacknowledged inputs
3. **Rewinding** - When server state arrives, rewind to that authoritative position
4. **Replaying** - Replay all unacknowledged inputs from that point
5. **Smooth correction** - Small errors are ignored to avoid jitter

## Architecture

### InputHistoryComponent

Stores a history of inputs sent to the server but not yet acknowledged:

```cpp
struct InputHistoryEntry {
    std::uint32_t sequenceId;  // Unique ID for this input
    std::uint16_t flags;        // Movement/action flags
    float posX, posY, angle;    // State when input was sent
    float deltaTime;            // Time step for replay
};

struct InputHistoryComponent {
    std::deque<InputHistoryEntry> history;
    std::uint32_t lastAcknowledgedSequence;
    std::size_t maxHistorySize = 128;

    void pushInput(uint32_t seq, uint16_t flags, ...);
    void acknowledgeUpTo(uint32_t seq);
    std::deque<InputHistoryEntry> getInputsAfter(uint32_t seq);
};
```

### ReconciliationSystem

Manages the rewind-and-replay process:

```cpp
class ReconciliationSystem : public ISystem {
public:
    void reconcile(Registry& registry,
                   EntityId entityId,
                   float authoritativeX,
                   float authoritativeY,
                   uint32_t acknowledgedSequence);
};
```

## Usage

### 1. Setup Input History

When creating a player entity that needs prediction:

```cpp
EntityId player = registry.createEntity();
registry.emplace<TransformComponent>(player);
registry.emplace<InputHistoryComponent>(player);
```

### 2. Store Inputs When Sending

When sending input to server, also store in history:

```cpp
// In InputSystem or similar
void sendInput(uint16_t flags, float posX, float posY, float angle) {
    uint32_t sequenceId = nextSequence();

    // Send to server
    InputPacket packet;
    packet.header.sequenceId = sequenceId;
    packet.flags = flags;
    packet.x = posX;
    packet.y = posY;
    packet.angle = angle;
    sendPacket(packet);

    // Store in history for reconciliation
    if (registry.has<InputHistoryComponent>(playerEntity)) {
        auto& history = registry.get<InputHistoryComponent>(playerEntity);
        history.pushInput(sequenceId, flags, posX, posY, angle, deltaTime);
    }
}
```

### 3. Reconcile When Server State Arrives

When receiving authoritative state from server:

```cpp
// In your network receive handler or ReplicationSystem
void onServerStateReceived(EntityId entity, float serverX, float serverY,
                           uint32_t lastProcessedInputSeq) {
    reconciliationSystem.reconcile(registry, entity,
                                   serverX, serverY,
                                   lastProcessedInputSeq);
}
```

## How It Works

### Example Timeline

```
Time  Client                          Server
----  ------                          ------
t=0   Input seq=1: Move Right →
      Predict: pos = (5, 0)

t=16  Input seq=2: Move Right →       Receives seq=1
      Predict: pos = (10, 0)          Process seq=1
                                      pos = (5, 0)

t=32  Input seq=3: Move Right →       Receives seq=2
      Predict: pos = (15, 0)          Process seq=2
                                      pos = (10, 0)
                                      ← Sends state: pos=(10,0), lastSeq=2

t=48  Receives server state!
      Current prediction: (15, 0)
      Server says: (10, 0) @ seq=2

      Reconciliation:
      1. Rewind to (10, 0)
      2. Replay seq=3
      3. Result: (15, 0) ✓
      4. Acknowledge seq=1,2
```

### With Prediction Error

If the client and server disagree (e.g., due to collision):

```
t=48  Receives server state
      Current prediction: (15, 0)
      Server says: (9, 0) @ seq=2    ← Collision stopped movement!

      Reconciliation:
      1. Rewind to (9, 0)            ← Use authoritative position
      2. Replay seq=3
      3. Result: (14, 0)             ← Corrected prediction
      4. Visual change: 15→14        ← Smooth 1-unit correction
```

## Configuration

### Movement Speed

**Critical**: Client and server must use the same movement speed!

```cpp
// Client
ReconciliationSystem reconciliation;
reconciliation.playerMoveSpeed_ = 200.0F;  // Must match server

// Server (in movement system)
const float PLAYER_SPEED = 200.0F;         // Must match client
```

### Reconciliation Threshold

Controls when to apply corrections (avoids jitter from small errors):

```cpp
reconciliation.reconciliationThreshold_ = 0.5F;  // Ignore errors < 0.5 units
```

**Lower values**: More accurate but potential jitter
**Higher values**: Smoother but allows more error

Recommended: 0.5 - 2.0 units depending on game speed

### History Size

Maximum number of stored inputs:

```cpp
InputHistoryComponent history;
history.maxHistorySize = 128;  // ~2 seconds at 60fps
```

**Too small**: Old inputs lost if high latency
**Too large**: Memory waste

Recommended: 128-256 (supports 500ms-1s RTT)

## Integration with Existing Systems

### With ReplicationSystem

The ReplicationSystem receives server snapshots. Extend it to trigger reconciliation:

```cpp
// In ReplicationSystem::applyTransform()
void ReplicationSystem::applyTransform(Registry& registry, EntityId id,
                                       const SnapshotEntity& entity) {
    if (!entity.posX.has_value() && !entity.posY.has_value()) {
        return;
    }

    // Check if this is a player-controlled entity
    if (registry.has<InputHistoryComponent>(id)) {
        // Use reconciliation instead of direct position update
        reconciliationSystem_->reconcile(
            registry, id,
            entity.posX.value_or(0.0F),
            entity.posY.value_or(0.0F),
            entity.lastProcessedInput  // Server must send this!
        );
    } else {
        // Non-player entities: direct update
        auto& transform = registry.get<TransformComponent>(id);
        if (entity.posX.has_value()) transform.x = *entity.posX;
        if (entity.posY.has_value()) transform.y = *entity.posY;
    }
}
```

### With InputSystem

Store inputs when sending:

```cpp
void InputSystem::update(Registry& registry, float deltaTime) {
    uint16_t flags = mapper_->getFlags();
    float angle = calculateAngle();

    uint32_t seq = nextSequence();

    // Send to server
    InputCommand cmd;
    cmd.sequenceId = seq;
    cmd.flags = flags;
    cmd.posX = *posX_;
    cmd.posY = *posY_;
    cmd.angle = angle;
    buffer_->push(cmd);

    // Store for reconciliation (player entity assumed to be ID 1)
    EntityId player = getLocalPlayer(registry);
    if (registry.has<InputHistoryComponent>(player)) {
        auto& history = registry.get<InputHistoryComponent>(player);
        history.pushInput(seq, flags, *posX_, *posY_, angle, deltaTime);
    }
}
```

## Server Requirements

For reconciliation to work, the server must:

1. **Track last processed input** per client
2. **Send this sequence ID** in state updates
3. **Use deterministic simulation** (same as client)

Example server state packet extension:

```cpp
struct PlayerStateUpdate {
    uint32_t entityId;
    float x, y;
    uint32_t lastProcessedInputSeq;  // ← Required for reconciliation
};
```

## Best Practices

### 1. Deterministic Simulation

Client and server movement logic MUST be identical:

```cpp
// ✓ Good: Same code on client and server
void applyMovement(Transform& t, uint16_t flags, float dt, float speed) {
    float dx = 0, dy = 0;
    if (flags & MoveUp) dy -= 1;
    if (flags & MoveDown) dy += 1;
    // ...
    t.x += dx * speed * dt;
    t.y += dy * speed * dt;
}

// ✗ Bad: Different logic
// Client: instant movement
// Server: accelerated movement
```

### 2. Input Sequence Management

Use monotonically increasing sequence IDs:

```cpp
uint32_t sequenceCounter = 0;
uint32_t nextSequence() {
    return ++sequenceCounter;
}
```

Never reuse or skip sequence IDs!

### 3. Handling Packet Loss

If server acknowledges seq=10 but client never sent seq=8:

```cpp
// In reconciliation
void acknowledgeUpTo(uint32_t seq) {
    // Remove ALL inputs <= seq, even if some were lost
    while (!history.empty() && history.front().sequenceId <= seq) {
        history.pop_front();
    }
}
```

This is safe because server is authoritative - if an input was lost, server didn't process it, so we shouldn't replay it.

### 4. Delta Time Accuracy

Store delta time with each input for accurate replay:

```cpp
// When sending input
history.pushInput(seq, flags, x, y, angle, deltaTime);

// Server should also use same delta time
// OR client can store server tick rate
```

### 5. Reconciliation Threshold Tuning

Test different thresholds:

```cpp
// High-speed game (fast movement)
threshold = 2.0F;  // Ignore small errors

// Precision game (slow, tactical)
threshold = 0.1F;  // Very accurate

// Test by adding artificial lag
```

## Debugging

### Visualizing Reconciliation

Add debug rendering to see when reconciliation occurs:

```cpp
void ReconciliationSystem::reconcile(...) {
    float errorMagnitude = std::sqrt(errorX * errorX + errorY * errorY);

    #ifdef DEBUG_RECONCILIATION
    if (errorMagnitude >= reconciliationThreshold_) {
        std::cout << "Reconciliation: error=" << errorMagnitude
                  << " rewind to (" << authoritativeX << "," << authoritativeY << ")"
                  << " replay " << unacknowledgedInputs.size() << " inputs"
                  << std::endl;
    }
    #endif

    // ...
}
```

### Common Issues

**Jittery movement**:
- Threshold too low → increase reconciliationThreshold_
- Different delta times → ensure client and server use same dt

**Rubber-banding**:
- Movement speed mismatch → verify client/server use same speed
- Missing inputs → check input history size

**Inputs not acknowledged**:
- Server not sending lastProcessedInputSeq → update server protocol
- Sequence ID mismatch → verify sequence generation

## Performance Considerations

### Memory

Each input entry: ~32 bytes
With 128 max history: 4 KB per player

For 100 players with prediction: 400 KB total

### CPU

Reconciliation cost:
- Threshold check: O(1)
- Rewind: O(1)
- Replay: O(n) where n = unacknowledged inputs

Typical case: 2-5 inputs to replay (60fps, 100ms RTT)
Worst case: 32 inputs (60fps, 500ms RTT)

Very cheap compared to rendering!

## Future Enhancements

### Smooth Error Correction

Instead of instant rewind, lerp over a few frames:

```cpp
void reconcile(...) {
    float errorX = predictedX - authoritativeX;

    if (errorMagnitude > threshold) {
        // Store error for gradual correction
        transform.correctionX = -errorX;
        transform.correctionSpeed = 5.0F;  // Correct over 5 frames
    }
}
```

### Entity Interpolation

Combine with interpolation for non-player entities:

```cpp
if (isLocalPlayer) {
    reconcile();  // Prediction + reconciliation
} else {
    interpolate();  // Smooth interpolation
}
```

### Lag Compensation

For shooting/interactions, raycast using historical positions.

## References

- [Gabriel Gambetta - Fast-Paced Multiplayer](https://www.gabrielgambetta.com/client-side-prediction-server-reconciliation.html)
- [Valve - Source Multiplayer Networking](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking)
- [Glenn Fiedler - Networked Physics](https://gafferongames.com/post/networked_physics_2004/)
