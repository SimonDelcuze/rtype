# Replication System (Client)

Applies authoritative snapshots on the client ECS. Runs on the main thread after network message dispatch.

## Flow
- `NetworkReceiver` (thread) pushes raw packets → `ThreadSafeQueue<std::vector<uint8_t>>`.
- `NetworkMessageHandler` (main thread) pops raw packets, parses snapshots (`SnapshotParser`), and pushes `SnapshotParseResult` into a parsed queue.
- `ReplicationSystem` (main thread) drains the parsed queue, maps remote IDs to local entities, updates components, and seeds interpolation.

## Responsibilities
- Create entities if unseen using `entityType` from snapshots; instantiate archetypes from `EntityTypeRegistry` (sprite/layer, animation if `frameCount` > 1). Reuse mapping for known remote IDs.
- If a new entity arrives without `entityType` or with an unknown type, log a warning and skip creation.
- Apply `Transform` (posX/posY), `Velocity` (velX/velY), `Health` (current/max, never lowers max), and destroy entities when `dead` is set (also clears the remote→local mapping).
- Seed/update `InterpolationComponent` targets and velocities; reset elapsed time on new snapshots.
- Ignore unsupported fields (e.g., status) safely.

## Lifetime
- Instantiated with the parsed snapshot queue and the `EntityTypeRegistry`.
- Called each frame in the client scheduler; consumes all available snapshots.
- Clears its remote→local map on cleanup.

## Tests
- `tests/client/systems/ReplicationSystemTests.cpp` (17 tests) cover typed creation, unknown/missing types skipped, updates, destruction, interpolation reset, multiple snapshots, missing fields, velocity-only, transform-only, mapping reuse, and max health preservation.
