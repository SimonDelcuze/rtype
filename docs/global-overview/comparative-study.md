# Technical and Comparative Study

This document justifies the main technology choices of the RType project. C++ is imposed by the subject; the degrees of freedom are primarily the graphics layer and networking stack. The key axes below cover algorithms/data structures, storage, and security.

## Graphics Library Choice
- **Chosen**: SFML. Rationale: the team already has experience with SFML, minimizing ramp-up. It offers a straightforward API for 2D sprites/text/audio, easy integration via CMake FetchContent, and cross-platform support without heavy boilerplate.
- **Alternatives considered**:
  - SDL2: comparable portability/input; would require more code for rendering/text (SDL_ttf) and the team has less hands-on experience.
  - raylib: simple but thinner ecosystem and little team experience; weaker text/layout tooling.
- **Why SFML fits**: leverages existing expertise to ship faster while covering all 2D needs without custom rendering backends.

## Networking Stack Choice
- **Constraint**: UDP mandated by the subject.
- **Chosen**: Custom wrapper over OS UDP sockets with project-specific packet formats.
- **Alternatives considered**:
  - TCP: not allowed by the constraint; would add head-of-line blocking and higher latency.
  - ENet/reliable UDP: adds dependencies and abstracts the wire format, reducing control over layout/timing.
  - Boost.Asio: powerful async API but heavier dependency and boilerplate than needed for simple datagram loops.
- **Why a custom wrapper fits**: keeps full control over buffer layout, timing, and error handling while satisfying the UDP requirement. CRC32 and strict header validation mitigate corruption without extra dependencies.

## Algorithms and Architectures
- **ECS-style registry (chosen)**: entities/components/systems decouple data and behavior, making composition easy and batching/cache-friendly iteration possible.
- **Architectural alternatives**:
  - OO hierarchies/inheritance: simpler but rigid; hard to compose behaviors and batch updates.
  - Component-based without central ECS: ad hoc composition, poorer cache locality and consistency.
- **Snapshot encoding**: component-driven masks with optional chunking to limit bandwidth; full-state every tick would waste bandwidth, and full delta compression was overkill for the scope.
- **Input pipeline**: fixed-size bitmask flags keep packets small; textual formats (JSON) rejected for size/latency reasons.
- **Threading**: dedicated send/receive threads avoid blocking the game loop; simpler than an event-multiplexed single thread for our needs.
- **CRC32**: lightweight integrity check; crypto hashes would add overhead without matching threat model.

## Storage
- **Runtime state**: transient in-memory ECS registry.
- **Registry layout (chosen)**: per-component trio of sparse array + dense array + data array (SoA) to get O(1) membership checks, contiguous iteration, and stable handles. This balances fast lookups with cache-friendly iteration and cheap removals via swap-erase in the dense/data arrays.
- **Alternatives**:
  - Map/set per component: simpler API but slower iteration and worse cache locality.
  - Pure dense array without sparse indirection: faster iteration but unstable indices and expensive deletions for dynamic entities.
- **Persistence**: none (no DB); saves/leaderboards are out of scope.
- **Assets**: manifest-driven load via SFML; embedding or VFS not needed for current size.

## Security and Integrity
- **Validation**: every packet validated for magic/version, payload size, message type/direction, and CRC32; tests cover size/type/CRC rejection paths.
- **Surface reduction**: binary protocol with fixed headers; unknown bits are ignored safely instead of crashing.
- **Known gaps**: no auth/encryption; suitable only for trusted/basic multiplayer. No anti-cheat beyond sanity checks. Logging is the main long-term monitoring; no IDS.
- **Mitigations considered**: rate limiting, optional HMAC/auth, and broader fuzzing (NaN/overflow) in future iterations.

## Summary
SFML plus custom UDP gives a small, controllable stack aligned with the subject constraints and real-time needs. ECS-style organization and component masks keep bandwidth and code manageable. Storage is intentionally minimal. Security focuses on integrity checks and validation; stronger guarantees (auth/encryption) remain future work if deployment requires it.
