# Registry



The **Registry** is the core of our ECS architecture.\
It manages:

* Entity creation & destruction
* Component storage
* Signatures (component bitmasks)
* Views for system iteration
* Event connections with systems

The entire engine — both server and client — relies on the Registry to access the game world efficiently and deterministically.

## **1. Entities**

Entities are **simple integer IDs** (`uint32_t`).\
The Registry tracks them using:

* A dense list of active entity IDs
* A boolean array `alive[]`
* A recycled ID stack for destroyed entities

#### Entity Lifecycle

```
Entity e = registry.createEntity();
registry.destroyEntity(e);
```

Destroying an entity:

* Marks it as not alive
* Removes its components
* Returns its ID to the free list

**Entities themselves contain no data** — all data is stored in components.

***

## **2. Signatures (Component Masks)**

Each entity has a **signature**, a bitmask encoding which components it possesses.

Example (bit index = component type):

```
Transform = 0001
Velocity  = 0010
Hitbox    = 0100
Health    = 1000
```

#### Signature structure

```
std::bitset<MAX_COMPONENTS> signature;
```

#### Why signatures?

* Systems can filter entities at O(1)
* Fast determination of “does entity X have component Y?”
* Simplifies entity iteration logic

***

## **3. Component Storage (Sparse/Dense Arrays)**

Our ECS uses a **sparse/dense array storage**, identical to what high-performance engines use (EnTT, Flecs, Unity DOTS-like ECS).

#### Structure

* **Sparse array**: indexed by entity → index in dense or `npos`
* **Dense array**: packed list of entities that have the component
* **Component array**: same size as dense array, stores the component values

Example:

```
Sparse:  [npos, 0, 1, npos, 2, ...]
Dense:   [1, 2, 4]
Data:    [Transform(1), Transform(2), Transform(4)]
```

#### Benefits

✔ O(1) insertion/removal\
✔ Cache-friendly\
✔ No fragmentation\
✔ Perfect for server deterministic loops\
✔ Great for client interpolation

***

## **4. Adding & Removing Components**

#### Add a component

```cpp
registry.emplace<Transform>(entity, x, y, rotation);
```

#### Remove a component

```cpp
registry.remove<Velocity>(entity);
```

Registry automatically:

* Updates the entity’s signature
* Inserts the component in dense storage
* Maintains sparse lookups

***

## **5. Views**

A **View** iterates over all entities possessing a required set of components.

Example:

```cpp
for (auto [e, t, v] : registry.view<Transform, Velocity>()) {
    t.x += v.vx * delta;
    t.y += v.vy * delta;
}
```

The view:

* Filters using signatures
* Provides direct references to component instances
* Is the main interface systems use

Views guarantee:

* No boxing
* No virtual calls
* Minimal overhead

***

## **6. Performance Considerations**

The Registry is optimized for:

#### ✔ Server-side deterministic logic

At 60 FPS, the server:

* Moves entities
* Computes AI
* Resolves collisions\
  All using rapid iteration through dense arrays.

#### ✔ Client interpolation & rendering

The client:

* Interpolates transforms
* Updates animations
* Renders sorted entities

ECS ensures no duplicated state and fast cache-friendly reads.

#### ✔ Minimal allocations

Everything is stored in pre-reserved contiguous vectors.

***

## **7. Why This Registry Design?**

This design was chosen over OOP because:

* ECS performs **much faster** for many small entities (missiles, monsters)
* Server logic stays **pure**, **clean**, **predictable**
* Network replication becomes **trivial**\
  (we replicate components, not objects)

Compared to Unity-style ECS or Flecs:

* Ours is simpler
* Fully custom
* Tailored for R-Type needs

<br>
