# Systems Overview

Systems are the **logic layer** of the ECS architecture.\
They operate on entities by selecting them through **component combinations**, using ECS views.

A system is:

* Stateless (ideally)
* Pure logic
* Executed every frame
* Independent from other systems
* Driven by the Registry

Systems form the behavior of the engine, while the Registry and components provide the data structure.

***

## **1. What Is a System?**

A system:

* Declares which components it needs
* Iterates only on entities that have these components
* Reads and writes component data
* Does not store entity-specific state

All systems in our architecture implement the **`ISystem` interface**, which provides:
* A standardized `update(Registry&, float deltaTime)` method
* Optional lifecycle hooks (`onInit()`, `onShutdown()`)
* Compatibility with the scheduler

Example pattern:

```cpp
#include "systems/ISystem.hpp"

class MovementSystem : public ISystem {
public:
    void update(Registry& registry, float dt) override {
        for (EntityId entity : registry.view<Transform, Velocity>()) {
            auto& transform = registry.get<Transform>(entity);
            auto& velocity = registry.get<Velocity>(entity);
            
            transform.x += velocity.dx * dt;
            transform.y += velocity.dy * dt;
        }
    }
};
```

Systems are called in order by a **Scheduler** (see [Scheduler Architecture](scheduler-architecture.md)).

***

## **2. System Categories**

Even though each part of the project (server vs client) has its own systems, all systems fall into these common categories.

#### Simulation Systems

These run gameplay logic:

* AI updates
* Movement updates
* Collision checks
* Damage calculation
* Entity lifetimes

On the **server**, these are authoritative.\
On the **client**, they're used only for prediction/interpolation.

***

#### Rendering Systems (Client Only)

Rendering systems convert ECS data into:

* Sprites
* Animations
* Particles
* Drawing routines

These systems are not used server-side.

***

#### Networking Systems

Networking logic is typically not part of a system, but in our architecture:

* Some systems package ECS data for replication
* Some systems consume network snapshots and apply them to ECS state

Examples:

* Delta-state builder (server)
* Snapshot application system (client)

***

#### &#x20;Utility Systems

These systems maintain internal engine state:

* Timer systems
* Cleanup systems
* Destruction queues
* Input processing systems
* Event processing systems

***

## **3. System Iteration Model**

Systems iterate over **views**, which are constructed based on component requirements.

Example:

```cpp
registry.view<Transform, Velocity>();
registry.view<MonsterAI>();
registry.view<Hitbox, Health>();
```

Views provide:

* A list of matching entity IDs
* References to the required component instances
* Fast sequential access (dense arrays)

This is what makes the ECS extremely fast compared to OOP.

***

## **4. System Execution Order**

Systems run in a deterministic order.

#### **Server (Authoritative Game Loop)**

At 60 FPS:

1. Process player inputs
2. Apply input logic (movement, actions)
3. AI updates
4. Physics/movement
5. Collisions
6. Damage
7. Entity destruction
8. Entity spawns
9. Delta-state snapshot generation

#### **Client (Render Loop)**

At monitor FPS (e.g., 144 FPS):

1. Read local input
2. Apply prediction systems
3. Apply latest server snapshots
4. Correct prediction errors
5. Update animations
6. Render sprites
7. Update audio
8. Debug/overlay systems

**Observation:**\
Client and server share the same ECS _foundation_ but execute completely different system pipelines.

***

## **5. System Isolation**

To avoid coupling:

* Systems must not call each other directly
* No system should assume another has run unless enforced by the loop order
* Communication is done through:
  * Shared components
  * Event Bus (see next page)

This prevents spaghetti-logic and ensures systems remain modular.

***

## **6. Stateless vs Stateful Systems**

#### Stateless systems (recommended)

Operate _only_ on component data.

Most core simulation systems follow this.

#### Stateful systems (sometimes necessary)

Maintain internal timers, pools, or configuration.

Examples:

* AI systems holding pattern definitions
* Animation systems with timing logic
* Networking systems tracking last snapshot tick

State should always be external and not tied to a specific entity.

***

## **7. Advantages of System Architecture**

#### High performance

Systems iterate in tight loops through dense arrays.

#### Clean separation

Each game mechanic is isolated.

#### Easy to extend

Adding new gameplay:

* Add a component
* Add a system
* No need to rewrite anything else

#### Perfect for networking

Server simulation stays pure, deterministic, and replicable.
