# Input Pipeline (Client)

Captures local input, maps it to flags, queues commands, and prepares them for networking/prediction.

## Components
- `InputMapper`  
  - Polls keyboard state (Up/Down/Left/Right/Space) → bitmask flags.
- `InputBuffer`  
  - Thread-safe queue of `InputCommand { flags, sequenceId, posX, posY, angle }`.
- `InputSystem`  
  - Main-thread system that polls flags each frame, builds an `InputCommand` with a sequence counter and current player position, and pushes it into `InputBuffer`.

## Flow
1) `InputSystem` runs in the GameLoop: poll flags → compute angle → enqueue `InputCommand`.
2) Later, a `NetworkSender` (to be integrated) will drain `InputBuffer`, encode `InputPacket`, and send via UDP.
3) Prediction/Reconciliation systems can also consume buffered commands for local movement/replay.

## Notes
- Uses SFML keyboard polling (non-blocking).
- Sequence IDs are maintained in `Main` and injected into `InputSystem`.
- Position fields are provided by `Main` (to be wired with the local player state for prediction).

## Tests
- `tests/client/input/InputBufferTests.cpp` covers FIFO order, empty pop, multi-thread push, large volumes, and draining behavior.
