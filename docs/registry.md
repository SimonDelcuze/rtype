# ECS Registry Detailed Design

The shared registry in `shared/include/ecs/Registry.hpp` powers both client and server ECS code. This document explains each internal component.

## Entity Lifecycle

- `EntityId` is a 32-bit unsigned integer (`using EntityId = std::uint32_t`).
- `createEntity()` reuses a free ID when available, otherwise allocates a new slot, marks it alive, and clears its signature.
- `destroyEntity(id)` ignores invalid/dead IDs; otherwise it marks the slot as dead, removes every component via the storages, resets the signature, and pushes the ID into `freeIds_`.
- `isAlive(id)` simply checks the `alive_` array.
- `clear()` erases entities, signatures, storages, and resets `nextId_`.

## Component Type IDs

- `ComponentTypeId::value<T>()` returns a unique index per type using an internal atomic counter.
- This index selects the bit to toggle inside an entity signature and identifies the matching storage.

## Per-entity Signatures

- Each entity owns a bitset stored in the flattened `signatures_` vector (`uint64_t` words).
- `signatureWordCount_` is the number of 64-bit words allocated per entity.
- `ensureSignatureWordCount(componentIndex)` grows the signature capacity whenever a new component type needs an extra bit. Existing signatures are migrated into the new layout.
- `appendSignatureForNewEntity()` appends a zeroed signature whenever a fresh ID extends the entity array.
- `resetSignature(id)` clears all bits for an entity (called when the ID is destroyed or reused).
- `setSignatureBit`, `clearSignatureBit`, and `hasSignatureBit` apply the bit operations; the public API relies on them for O(1) membership checks.

## Sparse-set Component Storage

For each component type `T`, the registry keeps a `ComponentStorage<T>`:

- `dense`: compact list of entity IDs that own `T`.
- `data`: packed array of `T` instances aligned with `dense`.
- `sparse`: vector indexed by `EntityId` pointing at the position inside `dense` (or `npos` if absent).

`storages_` (a `std::unordered_map<std::type_index, std::unique_ptr<ComponentStorageBase>>`) stores every component storage. `findStorage<T>()` queries the map; `ensureStorage<T>()` creates a new storage when needed.

### Operations

- `emplace<T>(entity, args...)`:
  1. Validate that the entity is alive.
  2. Compute the component index and expand signatures if required.
  3. Fetch or create the `ComponentStorage<T>`.
  4. `ComponentStorage::emplace` inserts or overwrites the data inside the sparse-set (updates `dense`, `data`, `sparse`).
  5. Set the signature bit.
- `has<T>(entity)`: only checks the signature; if the bit is clear, no storage lookup occurs.
- `get<T>(entity)`: validates `alive` + signature, then returns `data[sparse[id]]`. Throws `ComponentNotFoundError` when missing.
- `remove<T>(entity)`: stops if the entity is dead or the bit is clear; otherwise `ComponentStorage::remove` erases the entry in O(1) (swap with last) and clears the signature bit.
- `destroyEntity(entity)`: resets the signature words and iterates over every storage via `ComponentStorageBase::remove` to drop remaining components.

## Public API & Errors

| Method | Behavior | Exceptions |
| --- | --- | --- |
| `emplace<C>(EntityId, Args&&...)` | Adds or replaces `C`, returns a reference to the stored instance. | `RegistryError` (dead entity) |
| `has<C>(EntityId)` | Checks presence of `C`. | None |
| `get<C>(EntityId)` / `const get` | Retrieves `C`. | `RegistryError` (dead entity) or `ComponentNotFoundError` |
| `remove<C>(EntityId)` | Removes `C` when present. | None |

Error types (in `shared/include/errors/`):
- `IError`: base interface with `message()`.
- `RegistryError`: thrown for lifecycle misuse (dead entity, etc.).
- `ComponentNotFoundError`: thrown when the requested component is missing.

## Tests

`tests/shared/ECS/RegistryTests.cpp` and `MovementComponentTests.cpp` cover:
- ID recycling and component cleanup when entities die.
- Successful `emplace/get/remove` flows.
- Exceptions when accessing dead entities or missing components.
- Sparse-set behavior (inserting via helpers and verifying stored values).

## Example

```cpp
Registry registry;
EntityId e = registry.createEntity();

registry.emplace<Position>(e, 10.f, 20.f);
registry.emplace<Health>(e, 100);

if (registry.has<Position>(e)) {
    auto& pos = registry.get<Position>(e);
    // ...
}

registry.remove<Health>(e);
registry.destroyEntity(e);
```

## Notes

- The registry is not thread-safe; external synchronization is required when mutating it from multiple threads.
- Per-entity signatures and sparse-set storage form the foundation for upcoming `view<Component...>` helpers and the system scheduler.
- Reference files: `shared/include/ecs/Registry.hpp/.tpp` (API/templates), `shared/src/ecs/Registry.cpp` (signatures + lifecycle), `shared/include/ecs/ComponentTypeId.hpp` and `.cpp` (type indices).
