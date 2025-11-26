# Replication System (Client)

Applies authoritative snapshots on the client ECS. Runs on the main thread after network message dispatch.

## Flow
- `NetworkReceiver` (thread) pushes raw packets → `ThreadSafeQueue<std::vector<uint8_t>>`.
- `NetworkMessageHandler` (main thread) pops raw packets, parses snapshots (`SnapshotParser`), and pushes `SnapshotParseResult` into a parsed queue.
- `ReplicationSystem` (main thread) drains the parsed queue, maps remote IDs to local entities, updates components, and seeds interpolation.

## Responsibilities
- Create entities if unseen; reuse mapping for known remote IDs.
- Apply `Transform` (posX/posY), `Velocity` (velX/velY), `Health` (current/max, never lowers max), and destroy entities when `dead` is set.
- Seed/update `InterpolationComponent` targets and velocities; reset elapsed time on new snapshots.
- Ignore unsupported fields (e.g., status) safely.

## Lifetime
- Instantiated with a reference to the parsed snapshot queue.
- Called each frame in the client scheduler; consumes all available snapshots.
- Clears its remote→local map on cleanup.

## Tests
- `tests/client/systems/ReplicationSystemTests.cpp` (15 tests) cover creation, updates, destruction, interpolation reset, multiple snapshots, missing fields, velocity-only, transform-only, mapping reuse, and max health preservation.
