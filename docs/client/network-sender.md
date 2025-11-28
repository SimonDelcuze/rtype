# Network Sender

Role: drain the client input queue and transmit `InputPacket` over UDP at a fixed cadence, without blocking the game loop. Runs on its own thread and reports errors via `IError`-based callbacks.

## Responsibilities
- Pop `InputCommand` from `InputBuffer` (single-producer/single-consumer).
- Build an `InputPacket` (flags, sequence ID, player ID, position, angle).
- Encode and send via `UdpSocket` to the server endpoint.
- Log/report send/open failures through a non-blocking error callback.
- Sleep to respect the configured send interval (default ~16 ms).

## Lifecycle
- Construct with `InputBuffer`, remote endpoint, player ID, interval, optional bind endpoint, and error callback.
- `start()` opens a non-blocking socket, spawns the sender thread, and returns false if already running or open fails.
- `stop()` requests shutdown, joins the thread, and closes the socket; safe to call multiple times.

## Threading
- Dedicated sender thread; game loop remains on the main thread.
- Uses `InputBuffer::pop()` (thread-safe) to read commands; no busy waiting thanks to sleep when work is done.

## Error handling
- Errors surface through `onError(const IError&)`:
  - `NetworkSendError`: socket open/setup failures.
  - `NetworkSendSocketError`: sendTo failures.
- Callback is invoked inline on the sender thread; ensure handler stays lightweight.

## Wire format
- Uses existing `InputPacket::encode()`. Sequence ID truncated to 16 bits per protocol; player ID copied as provided.

## Integration (Main)
- `NetPipelines` now owns `sender`.
- `startSender(...)` builds the sender with:
  - `remote = 127.0.0.1:50001` (temporary default, adjust to server addr/port)
  - `interval = 16ms`
  - error callback printing to stderr
- On shutdown, main stops the sender then the receiver.

## Files
- Code: `client/include/network/NetworkSender.hpp`, `client/src/network/NetworkSender.cpp`
- Errors: `client/include/errors/NetworkSendError.hpp`
- Tests: `tests/client/network/NetworkSenderTests.cpp`
