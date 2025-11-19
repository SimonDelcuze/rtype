# ECS Registry Overview

This document summarizes the structure and behavior of the shared ECS registry implementation.

## Entity Lifecycle

- `EntityId` is a 32-bit unsigned integer (`using EntityId = std::uint32_t`).
- Call `Registry::createEntity()` to obtain a new ID; IDs are recycled when entities are destroyed.
- `Registry::destroyEntity(id)` marks an entity as dead, releases its components, and makes the ID available for reuse.
- `Registry::isAlive(id)` returns whether a given ID currently refers to a living entity.
- `Registry::clear()` wipes the entire registry (entities, components, and storage).

## Component Storage Skeleton

- Each component type gets its own `ComponentStorage<T>` instance, managed internally.
- Storage uses an `unordered_map<EntityId, T>` per component type (sparse-set style).
- Storages are created lazily when `emplace<T>` is executed for the first time.

## Registry API

All public methods perform validation and throw custom errors derived from `IError`:

| Method | Description | Throws |
|--------|-------------|--------|
| `template<typename C, typename... Args> C& emplace(EntityId, Args&&...)` | Adds/replaces component `C` on an entity. Returns a reference to the stored component. | `RegistryError` if entity is dead |
| `template<typename C> bool has(EntityId)` | Returns true if entity currently has component `C`. | *None* |
| `template<typename C> C& get(EntityId)` / `const C& get` | Retrieves a reference to component `C`. | `RegistryError` (dead entity) or `ComponentNotFoundError` (type missing) |
| `template<typename C> void remove(EntityId)` | Removes component `C` from an entity if present. | *None* |

### Error Types

Located in `shared/include/errors/`:

- `IError`: base interface containing the `message()` accessor.
- `RegistryError`: thrown for lifecycle-related issues (e.g. using dead entities).
- `ComponentNotFoundError`: thrown when a component is requested but not registered on the entity.

Both concrete errors are simple wrappers around a string message; any code can catch either specific errors or the `IError` base.

## Storage Internals

- `ComponentStorageBase`: abstract base used to hold different component storages in a homogeneous container.
- `ComponentStorage<C>` implements `emplace`, `contains`, `fetch`, `remove` for component `C`.
- `Registry` maintains `std::unordered_map<std::type_index, std::unique_ptr<ComponentStorageBase>> storages_` for dynamic dispatch.
- Each entity has a slot in `alive_` and a free-list for ID reuse.

## Unit Tests

Located in `tests/shared/ECS/RegistryTests.cpp` covering:

- ID reuse and entity destruction cleanup.
- Successful `emplace/get/remove` flows.
- Exceptions when acting on dead entities or missing components.
- Graceful behavior when removing a component that isn’t present.

## Usage Example

```cpp
Registry registry;
EntityId player = registry.createEntity();

registry.emplace<Position>(player, 10.f, 20.f);
registry.emplace<Health>(player, 100);

if (registry.has<Position>(player)) {
    auto& pos = registry.get<Position>(player);
    /* ... */
}

registry.remove<Health>(player);
registry.destroyEntity(player);
```

This skeleton should serve client and server code equally; higher-level systems can now build on top of it (rendering, networking, gameplay).

## Thread Safety

The registry implementation is currently *not* thread-safe; concurrent access to methods like `createEntity` or `destroyEntity` will need external synchronization if you plan to mutate the registry from multiple threads.
