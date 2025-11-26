# NetworkReceiver (Client)

The client spawns a dedicated UDP receive thread that listens for server snapshots and forwards raw packets to the main thread through a thread-safe queue.

## Thread model
- A `NetworkReceiver` owns a `UdpSocket` bound to an IPv4 endpoint (port configurable). It runs a background loop in its own thread.
- The receive thread never touches the ECS. It pushes received snapshot packets into a `ThreadSafeQueue<std::vector<std::uint8_t>>`.
- The main thread (e.g., a replication system) pops from the queue and applies decoded snapshots to the registry.

## Packet filtering
- Accepts only `PacketType::ServerToClient` and `MessageType::Snapshot`.
- Rejects invalid magic, protocol version, wrong packet type/message type, or too-short payloads.
- If a CRC is present, it is kept in the forwarded buffer; the handler may validate it downstream.

## Lifecycle
1) Construct `NetworkReceiver` with a bind endpoint and a handler that enqueues the raw packet.
2) Call `start()` to open the socket (non-blocking) and launch the thread.
3) Run the game loop; the replication system reads from the queue.
4) On shutdown, call `stop()` to join the thread and close the socket.

## Integration example
```cpp
ThreadSafeQueue<std::vector<std::uint8_t>> snapshotQueue;
NetworkReceiver receiver(IpEndpoint::v4(0, 0, 0, 0, 50000),
    [&](std::vector<std::uint8_t>&& pkt) { snapshotQueue.push(std::move(pkt)); });
receiver.start();
// Game loop...
receiver.stop();
```

## Tests
- `tests/client/network/NetworkReceiverTests.cpp` covers acceptance, filtering, trimming, and robustness (invalid magic/version, wrong packet type/message type, truncated header, short payload, multiple packets).
