# Registry

The **Registry** is the core of our ECS architecture, located in `shared/include/ecs/Registry.hpp`.\
It manages:

* Entity creation & destruction
* Component storage using sparse-set
* Signatures (component bitmasks) for fast queries
* Views for system iteration
* Thread-safe component access

The entire engine — both server and client — relies on the Registry to access the game world efficiently and deterministically.

---

## **1. Entity Lifecycle**

Entities are **simple 32-bit unsigned integer IDs** (`using EntityId = std::uint32_t`).\
The Registry tracks them using:

* A dense list of active entity IDs
* A boolean array `alive_[]` to mark living entities
* A recycled ID stack (`freeIds_`) for destroyed entities

### Creating Entities

```cpp
EntityId createEntity();
```

**Behavior:**
* **Reuses** a free ID from `freeIds_` when available
* **Allocates** a new slot if no recycled IDs exist
* Marks the entity as alive in `alive_[]`
* Clears its signature (no components initially)
* Returns the new `EntityId`

Example:
```cpp
Registry registry;
EntityId player = registry.createEntity();
EntityId enemy = registry.createEntity();
```

### Destroying Entities

```cpp
void destroyEntity(EntityId id);
```

**Behavior:**
1. Validates that the entity ID is valid and alive
2. Marks the entity as dead in `alive_[]`
3. Removes **all components** via component storages
4. Resets the entity's signature to zero
5. Pushes the ID into `freeIds_` for reuse

Example:
```cpp
registry.destroyEntity(enemy);  // Entity ID can be reused
```

### Checking Entity Status

```cpp
bool isAlive(EntityId id) const;
```

Returns `true` if the entity exists and is alive.

Example:
```cpp
if (registry.isAlive(player)) {
    // Safe to access components
}
```

### Clearing All Entities

```cpp
void clear();
```

Erases all entities, signatures, component storages, and resets `nextId_`.

**Entities themselves contain no data** — all data is stored in components.

---

## **2. Component Type IDs**

Each component type gets a **unique index** used for:
* Selecting the bit in an entity's signature
* Identifying the corresponding component storage

### Implementation

```cpp
// In ComponentTypeId.hpp
template <typename T>
static std::size_t ComponentTypeId::value();
```

**How it works:**
* Uses an internal **atomic counter**
* First call for type `T` assigns the next available ID
* Subsequent calls return the same ID
* Thread-safe via `std::atomic`

Example:
```cpp
std::size_t posId = ComponentTypeId::value<Position>();    // e.g., 0
std::size_t velId = ComponentTypeId::value<Velocity>();    // e.g., 1
std::size_t hpId = ComponentTypeId::value<Health>();       // e.g., 2
```

This index is used internally by the Registry to manage signatures and storages.

---

## **3. Per-Entity Signatures (Component Bitmasks)**

Each entity has a **signature**, a bitset encoding which components it possesses.

### Signature Structure

* Stored as a flattened array of `uint64_t` words in `signatures_`
* `signatureWordCount_` = number of 64-bit words allocated per entity
* Each bit represents one component type

Example (conceptual):
```
Position  = bit 0 → 0001
Velocity  = bit 1 → 0010
Hitbox    = bit 2 → 0100
Health    = bit 3 → 1000

Entity with Position + Velocity → signature = 0011
```

### Signature Operations

#### Growing Signature Capacity

```cpp
void ensureSignatureWordCount(std::size_t componentIndex);
```

* Called when a new component type needs more bits
* Expands the signature array
* Migrates existing signatures to the new layout

#### Entity Signature Management

```cpp
void appendSignatureForNewEntity();      // Add zeroed signature for new entity
void resetSignature(EntityId id);        // Clear all bits (on destroy)
void setSignatureBit(EntityId id, std::size_t componentIndex);    // Set bit
void clearSignatureBit(EntityId id, std::size_t componentIndex);  // Clear bit
bool hasSignatureBit(EntityId id, std::size_t componentIndex);    // Check bit (public)
```

### Why Signatures?

✅ **O(1) component checks** — systems can filter entities instantly\
✅ **Fast Views** — iterate only entities with required components\
✅ **Cache-friendly** — bitwise operations are extremely fast

---

## **4. Sparse-Set Component Storage**

For each component type `T`, the registry maintains a `ComponentStorage<T>` using the **sparse-set** data structure.

### Structure

```cpp
template <typename Component>
struct ComponentStorage {
    std::vector<EntityId> dense;      // Packed entity IDs
    std::vector<Component> data;      // Component instances (aligned with dense)
    std::vector<std::size_t> sparse;  // EntityId → index in dense (or npos)
    
    static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();
};
```

Example:
```
Entity 1 has Position
Entity 2 has Position
Entity 4 has Position

Sparse:  [npos, 0, 1, npos, 2, ...]
         (index by EntityId)
Dense:   [1, 2, 4]
         (packed entity IDs)
Data:    [Position(1), Position(2), Position(4)]
         (component values)
```

### Benefits

✅ **O(1) insertion/removal** — swap with last element\
✅ **Cache-friendly iteration** — dense array is contiguous\
✅ **No fragmentation** — no memory holes\
✅ **Minimal memory** — only stores entities that have the component

### Storage Management

All component storages are kept in:
```cpp
std::unordered_map<std::type_index, std::unique_ptr<ComponentStorageBase>> storages_;
```

Helper methods:
```cpp
template <typename Component>
ComponentStorage<Component>* findStorage();        // Query storage

template <typename Component>
ComponentStorage<Component>* ensureStorage();      // Get or create storage
```

---

## **5. Component Operations**

### Adding/Replacing a Component

```cpp
template <typename Component, typename... Args>
Component& emplace(EntityId id, Args&&... args);
```

**Behavior:**
1. Validates that the entity is alive (throws `RegistryError` if dead)
2. Computes the component index via `ComponentTypeId::value<Component>()`
3. Expands signatures if needed (`ensureSignatureWordCount`)
4. Gets or creates the `ComponentStorage<Component>`
5. Calls `ComponentStorage::emplace` to insert/update the component
6. Sets the signature bit for this component
7. Returns a reference to the stored component

Example:
```cpp
registry.emplace<Position>(player, 100.0f, 200.0f);
registry.emplace<Health>(player, 100);
```

### Checking Component Presence

```cpp
template <typename Component>
bool has(EntityId id) const;
```

**Fast path:** Only checks the signature bit — **no storage lookup**.

Example:
```cpp
if (registry.has<Velocity>(player)) {
    // Player is moving
}
```

### Retrieving a Component

```cpp
template <typename Component>
Component& get(EntityId id);

template <typename Component>
const Component& get(EntityId id) const;
```

**Behavior:**
1. Validates entity is alive (throws `RegistryError`)
2. Checks signature bit (throws `ComponentNotFoundError` if missing)
3. Returns `data[sparse[id]]` from the component storage

Example:
```cpp
auto& pos = registry.get<Position>(player);
pos.x += 10.0f;
```

### Removing a Component

```cpp
template <typename Component>
void remove(EntityId id);
```

**Behavior:**
1. Checks if entity is alive and has the component
2. Calls `ComponentStorage::remove` (swap-and-pop for O(1) removal)
3. Clears the signature bit

Safe to call even if the component doesn't exist (no-op).

Example:
```cpp
registry.remove<Velocity>(player);  // Stop movement
```

---

## **6. Views for System Iteration**

A **View** efficiently iterates over all entities possessing a required set of components.

### Creating a View

```cpp
template <typename... Components>
View<Components...> view();
```

Returns a `View` object that can be iterated with a range-based `for` loop.

### Using Views

```cpp
// Iterate over all entities with Position and Velocity
for (EntityId id : registry.view<Position, Velocity>()) {
    auto& pos = registry.get<Position>(id);
    auto& vel = registry.get<Velocity>(id);
    
    pos.x += vel.dx * deltaTime;
    pos.y += vel.dy * deltaTime;
}
```

### How Views Work

1. **Compute component indices** at construction
2. **Filter entities** using signature matching during iteration
3. **Skip dead entities** automatically
4. **Yield EntityId** for each matching entity

### View Performance

✅ **No boxing** — direct component access\
✅ **No virtual calls** — template-based\
✅ **Cache-friendly** — iterates dense storage\
✅ **Minimal overhead** — signature checks are O(1) bitwise ops

See the [View/Iterator documentation](TODO) for implementation details.

---

## **7. Public API Reference**

| Method | Description | Throws |
|--------|-------------|--------|
| `createEntity()` | Creates a new entity | - |
| `destroyEntity(EntityId)` | Destroys an entity and all its components | - |
| `isAlive(EntityId)` | Checks if entity exists | - |
| `clear()` | Removes all entities | - |
| `entityCount()` | Returns total entity count | - |
| `emplace<C>(EntityId, Args...)` | Adds/replaces component `C` | `RegistryError` (dead entity) |
| `has<C>(EntityId)` | Checks if entity has component `C` | - |
| `get<C>(EntityId)` | Retrieves component `C` | `RegistryError`, `ComponentNotFoundError` |
| `remove<C>(EntityId)` | Removes component `C` | - |
| `view<C1, C2, ...>()` | Creates a view to iterate entities | - |

### Error Types

Located in `shared/include/errors/`:

* **`IError`**: Base interface with `message()` method
* **`RegistryError`**: Thrown for lifecycle misuse (dead entity operations)
* **`ComponentNotFoundError`**: Thrown when requesting a missing component

---

## **8. Complete Example**

```cpp
#include "ecs/Registry.hpp"

struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Health { int hp; };

int main() {
    Registry registry;
    
    // Create entities
    EntityId player = registry.createEntity();
    EntityId enemy = registry.createEntity();
    
    // Add components
    registry.emplace<Position>(player, 100.0f, 200.0f);
    registry.emplace<Velocity>(player, 5.0f, 0.0f);
    registry.emplace<Health>(player, 100);
    
    registry.emplace<Position>(enemy, 500.0f, 300.0f);
    registry.emplace<Health>(enemy, 50);
    
    // Check component presence
    if (registry.has<Velocity>(player)) {
        std::cout << "Player can move\n";
    }
    
    // Retrieve and modify components
    auto& playerPos = registry.get<Position>(player);
    playerPos.x += 10.0f;
    
    // Iterate with Views
    for (EntityId id : registry.view<Position, Health>()) {
        auto& pos = registry.get<Position>(id);
        auto& hp = registry.get<Health>(id);
        std::cout << "Entity at (" << pos.x << "," << pos.y << ") with " << hp.hp << " HP\n";
    }
    
    // Remove components
    registry.remove<Velocity>(player);
    
    // Destroy entities
    registry.destroyEntity(enemy);
    
    return 0;
}
```

---

## **9. Performance Characteristics**

### Server-Side (60 FPS tick rate)

The Registry enables:
* **Fast AI updates** — iterate monsters with `view<Position, MonsterAI>()`
* **Efficient collision** — iterate `view<Position, Hitbox>()`
* **Deterministic** — consistent execution order

### Client-Side

The Registry supports:
* **Smooth interpolation** — `view<Position, NetworkTransform>()`
* **Animation updates** — `view<Sprite, AnimationComponent>()`
* **Rendering** — `view<Position, Sprite>()`

### Memory Efficiency

* **Minimal allocations** — pre-reserved vectors
* **No fragmentation** — densely packed component storage
* **Low overhead** — signatures are compact bitsets

---

## **10. Thread Safety**

⚠️ **The Registry is NOT thread-safe**

External synchronization is required when:
* Multiple threads modify entities
* One thread reads while another writes

**Recommendation:** Use one Registry per game instance, accessed only from the game loop thread.

---

## **11. Testing**

Unit tests are located in `tests/shared/ecs/`:

* **RegistryTests.cpp** — Entity lifecycle, component operations
* **ViewTests.cpp** — View/Iterator functionality
* **MovementComponentTests.cpp** — Component-specific tests

Tests cover:
* ✅ Entity ID recycling
* ✅ Component insertion/removal
* ✅ Exception handling (dead entities, missing components)
* ✅ Sparse-set correctness
* ✅ View filtering and iteration

---

## **12. Why This Design?**

### Compared to Object-Oriented Hierarchy

❌ **OOP**: Deep inheritance, virtual calls, scattered data\
✅ **ECS**: Composition, cache-friendly, fast iteration

### Compared to Component-Based (Unity-style)

❌ **Unity**: GameObject container, component lists, slower iteration\
✅ **Our ECS**: Pure data-oriented, sparse-set storage, minimal overhead

### Compared to EnTT / Flecs

✅ **Tailored** — Designed specifically for R-Type requirements\
✅ **Simple** — No archetypes, no meta-programming complexity\
✅ **Educational** — Fully custom implementation\
✅ **Predictable** — Server authoritative, deterministic ticks

---

## **13. Related Documentation**

* [View/Iterator API](TODO) — Detailed view implementation
* [Component Type ID](TODO) — Type index generation
* [Components Overview](components-overview.md) — All available components
* [Systems Overview](systems-overview.md) — How systems use the Registry

---

## **14. Source Files**

* **Header:** `shared/include/ecs/Registry.hpp`
* **Template implementation:** `shared/include/ecs/Registry.tpp`
* **Non-template methods:** `shared/src/ecs/Registry.cpp`
* **Component Type ID:** `shared/include/ecs/ComponentTypeId.hpp` + `.cpp`
* **Tests:** `tests/shared/ecs/RegistryTests.cpp`, `ViewTests.cpp`

