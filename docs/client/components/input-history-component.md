# InputHistoryComponent

## Overview

The `InputHistoryComponent` is a critical networking component in the R-Type client that maintains a history of player input states for client-side prediction and server reconciliation. It stores a rolling buffer of input snapshots with their associated sequence IDs, positions, and timing information, enabling smooth gameplay even under network latency conditions.

## Purpose and Design Philosophy

In networked multiplayer games, player input must be handled carefully:

1. **Client Prediction**: Apply inputs immediately for responsive gameplay
2. **Server Authority**: Server validates and authorizes all game state
3. **Reconciliation**: Correct client state when server disagrees
4. **Replay**: Re-apply unacknowledged inputs after correction
5. **Rollback**: Support rollback netcode for competitive play

The component acts as the client's input memory, storing everything needed to reconcile with authoritative server state.

## Component Structure

```cpp
struct InputHistoryEntry
{
    std::uint32_t sequenceId = 0;
    std::uint16_t flags      = 0;
    float posX               = 0.0F;
    float posY               = 0.0F;
    float angle              = 0.0F;
    float deltaTime          = 0.016F;
};

struct InputHistoryComponent
{
    std::deque<InputHistoryEntry> history;
    std::uint32_t lastAcknowledgedSequence = 0;
    std::size_t maxHistorySize             = 128;

    void pushInput(std::uint32_t sequenceId, std::uint16_t flags, 
                   float posX, float posY, float angle, float deltaTime = 0.016F);
    void acknowledgeUpTo(std::uint32_t sequenceId);
    std::deque<InputHistoryEntry> getInputsAfter(std::uint32_t sequenceId) const;
    void clear();
    std::size_t size() const;
};
```

### InputHistoryEntry Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `sequenceId` | `uint32_t` | `0` | Unique, incrementing identifier for this input frame. |
| `flags` | `uint16_t` | `0` | Bitfield of input state (movement, fire, etc.). |
| `posX` | `float` | `0.0F` | Player X position when input was applied. |
| `posY` | `float` | `0.0F` | Player Y position when input was applied. |
| `angle` | `float` | `0.0F` | Aim angle at time of input. |
| `deltaTime` | `float` | `0.016F` | Time delta for this input frame. |

### InputHistoryComponent Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `history` | `deque<InputHistoryEntry>` | `{}` | Rolling buffer of input entries. |
| `lastAcknowledgedSequence` | `uint32_t` | `0` | Latest sequence ID confirmed by server. |
| `maxHistorySize` | `size_t` | `128` | Maximum entries before oldest are discarded. |

## Methods

### `pushInput(...)`

Records a new input entry in the history.

```cpp
void pushInput(std::uint32_t sequenceId, std::uint16_t flags, 
               float posX, float posY, float angle, float deltaTime)
{
    InputHistoryEntry entry;
    entry.sequenceId = sequenceId;
    entry.flags      = flags;
    entry.posX       = posX;
    entry.posY       = posY;
    entry.angle      = angle;
    entry.deltaTime  = deltaTime;

    history.push_back(entry);

    // Maintain max size
    while (history.size() > maxHistorySize) {
        history.pop_front();
    }
}
```

### `acknowledgeUpTo(sequenceId)`

Removes all entries up to and including the acknowledged sequence.

```cpp
void acknowledgeUpTo(std::uint32_t sequenceId)
{
    lastAcknowledgedSequence = sequenceId;

    while (!history.empty() && history.front().sequenceId <= sequenceId) {
        history.pop_front();
    }
}
```

### `getInputsAfter(sequenceId)`

Returns all inputs after a given sequence ID for replay.

```cpp
std::deque<InputHistoryEntry> getInputsAfter(std::uint32_t sequenceId) const
{
    std::deque<InputHistoryEntry> result;

    for (const auto& entry : history) {
        if (entry.sequenceId > sequenceId) {
            result.push_back(entry);
        }
    }

    return result;
}
```

### `clear()`

Clears all history and resets state.

```cpp
void clear()
{
    history.clear();
    lastAcknowledgedSequence = 0;
}
```

## Input Flags Bitfield

Common input flag definitions:

```cpp
namespace InputFlags
{
    constexpr uint16_t None     = 0x0000;
    constexpr uint16_t Up       = 0x0001;
    constexpr uint16_t Down     = 0x0002;
    constexpr uint16_t Left     = 0x0004;
    constexpr uint16_t Right    = 0x0008;
    constexpr uint16_t Fire     = 0x0010;
    constexpr uint16_t Charge   = 0x0020;
    constexpr uint16_t Special  = 0x0040;
    constexpr uint16_t Pause    = 0x0080;
}
```

## Usage Patterns

### Recording Input Each Frame

```cpp
void InputSystem::processAndRecord(Registry& registry, Entity playerEntity,
                                     uint32_t currentSequence, float deltaTime)
{
    auto& history = registry.getComponent<InputHistoryComponent>(playerEntity);
    auto& transform = registry.getComponent<TransformComponent>(playerEntity);
    auto& input = registry.getComponent<PlayerInputComponent>(playerEntity);
    
    // Build input flags from current state
    uint16_t flags = 0;
    if (input.moveUp) flags |= InputFlags::Up;
    if (input.moveDown) flags |= InputFlags::Down;
    if (input.moveLeft) flags |= InputFlags::Left;
    if (input.moveRight) flags |= InputFlags::Right;
    if (input.fire) flags |= InputFlags::Fire;
    if (input.charge) flags |= InputFlags::Charge;
    
    // Record in history
    history.pushInput(
        currentSequence,
        flags,
        transform.x,
        transform.y,
        input.aimAngle,
        deltaTime
    );
}
```

### Server Acknowledgment Processing

```cpp
void NetworkReceiver::handleServerAck(Registry& registry, Entity playerEntity,
                                        const ServerAckPacket& packet)
{
    auto& history = registry.getComponent<InputHistoryComponent>(playerEntity);
    
    // Remove acknowledged inputs from history
    history.acknowledgeUpTo(packet.lastProcessedSequence);
    
    // Check for position mismatch
    if (std::abs(packet.serverX - packet.predictedX) > RECONCILIATION_THRESHOLD ||
        std::abs(packet.serverY - packet.predictedY) > RECONCILIATION_THRESHOLD) {
        // Trigger reconciliation
        reconcilePosition(registry, playerEntity, packet);
    }
}
```

### Client-Side Reconciliation

```cpp
void ReconciliationSystem::reconcile(Registry& registry, Entity playerEntity,
                                       const ServerAckPacket& serverState)
{
    auto& history = registry.getComponent<InputHistoryComponent>(playerEntity);
    auto& transform = registry.getComponent<TransformComponent>(playerEntity);
    auto& velocity = registry.getComponent<VelocityComponent>(playerEntity);
    
    // Reset to server authoritative position
    transform.x = serverState.serverX;
    transform.y = serverState.serverY;
    
    // Get all inputs after server's last processed sequence
    auto unprocessedInputs = history.getInputsAfter(serverState.lastProcessedSequence);
    
    // Replay each unprocessed input
    for (const auto& input : unprocessedInputs) {
        applyInput(transform, velocity, input.flags, input.deltaTime);
    }
}

void ReconciliationSystem::applyInput(TransformComponent& transform,
                                        VelocityComponent& velocity,
                                        uint16_t flags, float deltaTime)
{
    const float MOVE_SPEED = 200.0F;
    
    // Apply movement based on flags
    if (flags & InputFlags::Up) velocity.y = -MOVE_SPEED;
    else if (flags & InputFlags::Down) velocity.y = MOVE_SPEED;
    else velocity.y = 0;
    
    if (flags & InputFlags::Left) velocity.x = -MOVE_SPEED;
    else if (flags & InputFlags::Right) velocity.x = MOVE_SPEED;
    else velocity.x = 0;
    
    // Update position
    transform.x += velocity.x * deltaTime;
    transform.y += velocity.y * deltaTime;
}
```

## Network Integration

### Sending Input to Server

```cpp
void NetworkSender::sendInput(const InputHistoryEntry& input)
{
    InputPacket packet;
    packet.sequenceId = input.sequenceId;
    packet.flags = input.flags;
    packet.posX = input.posX;
    packet.posY = input.posY;
    packet.angle = input.angle;
    packet.deltaTime = input.deltaTime;
    
    m_socket.send(packet);
}
```

### Server Response Format

```cpp
struct ServerStatePacket
{
    uint32_t lastProcessedSequence;  // Last input server processed
    float serverX;                    // Authoritative X position
    float serverY;                    // Authoritative Y position
    // ... other state
};
```

## Sequence ID Management

```cpp
class InputSequencer
{
    uint32_t m_currentSequence = 0;
    
public:
    uint32_t next() { return ++m_currentSequence; }
    uint32_t current() const { return m_currentSequence; }
    void reset() { m_currentSequence = 0; }
};
```

## Buffer Size Considerations

| Scenario | Recommended Size | Rationale |
|----------|-----------------|-----------|
| Low Latency (<50ms) | 64 | Short history sufficient |
| Medium Latency (50-150ms) | 128 | Default size |
| High Latency (>150ms) | 256 | Need more rollback buffer |
| Rollback Netcode | 256+ | Fighting games need more |

## Best Practices

1. **Consistent Delta Time**: Record actual frame time, not fixed time step
2. **Position Snapshots**: Include position for validation
3. **Sequence Monotonicity**: Sequence IDs must always increase
4. **Buffer Sizing**: Size based on expected round-trip time
5. **Cleanup**: Clear history on disconnect/reconnect

## Performance Considerations

- **Deque Efficiency**: O(1) push/pop from both ends
- **Memory Bounded**: `maxHistorySize` prevents unbounded growth
- **Copy on Query**: `getInputsAfter` returns copies; consider iterators for hot paths

## Debugging

```cpp
void debugPrintHistory(const InputHistoryComponent& history)
{
    std::cout << "Input History (" << history.size() << " entries):\n";
    std::cout << "Last ACK: " << history.lastAcknowledgedSequence << "\n";
    
    for (const auto& entry : history.history) {
        std::cout << "  Seq " << entry.sequenceId 
                  << " Flags: 0x" << std::hex << entry.flags
                  << " Pos: (" << entry.posX << ", " << entry.posY << ")\n";
    }
}
```

## Related Components

- [PlayerInputComponent](../server/components/player-input-component.md) - Current input state
- [TransformComponent](../server/components/transform-component.md) - Position data
- [VelocityComponent](../server/components/velocity-component.md) - Movement state

## Related Systems

- [ReconciliationSystem](../systems/reconciliation-system.md) - Uses history for correction
- [NetworkSender](network-sender.md) - Sends inputs to server
- [NetworkReceiver](network-receiver.md) - Receives acknowledgments
