# PlayerInputComponent

**Location:** `shared/include/components/PlayerInputComponent.hpp` (planned)

The `PlayerInputComponent` stores the latest validated input state for a player entity. The server remains authoritative; these values are used for reconciliation and orientation.

---

## **Structure**

```cpp
struct PlayerInputComponent {
    float x = 0.0F;                // Client-reported position X (center)
    float y = 0.0F;                // Client-reported position Y (center)
    float angle = 0.0F;            // Client aim/ship angle (radians, 0 along +X, CCW)
    std::uint16_t sequenceId = 0;  // Last processed input sequence number
};
```

---

## **Fields**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `x` | `float` | `0.0F` | Client-reported position estimate (for reconciliation) |
| `y` | `float` | `0.0F` | Client-reported position estimate (for reconciliation) |
| `angle` | `float` | `0.0F` | Client aim/ship angle in radians |
| `sequenceId` | `uint16_t` | `0` | Sequence number for input ordering |

---

## **Invariants**

* Finite values for `x`, `y`, `angle`
* `sequenceId` monotonically increases per client

---

## **Usage**

### Server Receiving Input

```cpp
// Network thread receives UDP packet with input
PlayerInput incomingInput = deserializeInput(packet);

auto& input = registry.get<PlayerInputComponent>(playerId);

// Validate and apply
if (incomingInput.sequenceId > input.sequenceId) {
    input.x = incomingInput.x;
    input.y = incomingInput.y;
    input.angle = incomingInput.angle;
    input.sequenceId = incomingInput.sequenceId;
}
```

### PlayerSystem Processing

```cpp
for (EntityId id : registry.view<TransformComponent, PlayerInputComponent>()) {
    auto& transform = registry.get<TransformComponent>(id);
    auto& input = registry.get<PlayerInputComponent>(id);
    
    // Server-authoritative movement
    transform.rotation = input.angle;  // Apply client's aim direction
    
    // Validate and apply position (with server checks)
    if (isValidPosition(input.x, input.y)) {
        transform.x = input.x;
        transform.y = input.y;
    }
}
```

---

## **Networking**

* **Inbound only** — Received from owning client, never broadcast to others
* **Server validates** — All inputs checked for cheating/invalid values
* **Sequence IDs** — Prevent replay attacks and ensure ordering

---

## **Client-Server Reconciliation**

1. **Client sends:** Position + input + sequence ID
2. **Server validates:** Checks bounds, speed limits
3. **Server applies:** Updates authoritative position
4. **Server responds:** Sends confirmed position back
5. **Client reconciles:** Adjusts local simulation if mismatch

---

## **Systems**

* **NetworkSystem** — Receives and deserializes inputs
* **PlayerSystem** — Consumes inputs to update player state
* **MovementSystem** — May consult for movement validation

---

## **Related**

* [TransformComponent](transform-component.md)
* [Network Protocol](TODO)
