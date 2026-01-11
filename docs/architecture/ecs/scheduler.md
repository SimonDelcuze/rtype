# Scheduler Architecture

The **Scheduler** orchestrates the ECS game loop by executing systems in a deterministic order each frame. This page explains the scheduler and system interfaces that enable different implementations for client and server.

---

## **1. Overview**

The scheduler architecture consists of two pure interfaces:

* **`ISystem`** — Interface that all systems must implement
* **`IScheduler`** — Interface for scheduler implementations

This design allows:
* ✅ **Different implementations** for client and server
* ✅ **Shared contracts** through interfaces
* ✅ **Flexibility** while maintaining consistency
* ✅ **Testability** through dependency injection

---

## **2. The ISystem Interface**

Located in `shared/include/systems/ISystem.hpp`

### Purpose

`ISystem` defines the contract that all systems must follow. Systems implement game logic by operating on entities and components.

### Interface Definition

```cpp
class ISystem {
public:
    virtual ~ISystem() = default;
    
    virtual void initialize() {}
    virtual void update(Registry& registry, float deltaTime) = 0;
    virtual void cleanup() {}
};
```

### Methods

| Method | Required | Purpose |
|--------|----------|---------|
| `initialize()` | Optional | Called once when system is added to scheduler |
| `update()` | **Required** | Called every frame with registry and deltaTime |
| `cleanup()` | Optional | Called when scheduler stops |

### Creating a System

```cpp
#include "systems/ISystem.hpp"

class MovementSystem : public ISystem {
public:
    void update(Registry& registry, float deltaTime) override {
        for (EntityId entity : registry.view<Position, Velocity>()) {
            auto& pos = registry.get<Position>(entity);
            auto& vel = registry.get<Velocity>(entity);
            
            pos.x += vel.dx * deltaTime;
            pos.y += vel.dy * deltaTime;
        }
    }
};
```

### System Guidelines

**Keep systems stateless when possible:**
```cpp
// Good: Operates only on component data
class CollisionSystem : public ISystem {
    void update(Registry& registry, float deltaTime) override {
        // Check collisions using components only
    }
};
```

**Use state only when necessary:**
```cpp
// Acceptable: Maintains timing configuration
class SpawnSystem : public ISystem {
private:
    float spawnTimer_ = 0.0f;
    float spawnInterval_ = 2.0f;
    
public:
    void update(Registry& registry, float deltaTime) override {
        spawnTimer_ += deltaTime;
        if (spawnTimer_ >= spawnInterval_) {
            // Spawn enemy
            spawnTimer_ = 0.0f;
        }
    }
};
```

---

## **3. The IScheduler Interface**

Located in `shared/include/scheduler/IScheduler.hpp`

### Purpose

`IScheduler` defines the contract for scheduler implementations. Client and server create their own implementations with different characteristics.

### Interface Definition

```cpp
class IScheduler {
public:
    virtual ~IScheduler() = default;
    
    virtual void addSystem(std::shared_ptr<ISystem> system) = 0;
    virtual void update(Registry& registry, float deltaTime) = 0;
    virtual void stop() = 0;
};
```

### Why an Interface?

Different requirements for client and server:

| Context | Scheduler Type | Characteristics |
|---------|---------------|-----------------|
| **Server** | `ServerScheduler` | Fixed 60 Hz, deterministic, authoritative |
| **Client** | `ClientScheduler` | Variable FPS, prediction, interpolation |
| **Testing** | `TestScheduler` | Controllable, mockable |

---

## **4. Implementing a Scheduler**

### Example: Server Scheduler

```cpp
#include "scheduler/IScheduler.hpp"

class ServerScheduler : public IScheduler {
public:
    void addSystem(std::shared_ptr<ISystem> system) override {
        if (!system) {
            throw std::invalid_argument("Cannot add null system");
        }
        systems_.push_back(system);
        system->initialize();
    }
    
    void update(Registry& registry, float deltaTime) override {
        for (auto& system : systems_) {
            system->update(registry, deltaTime);
        }
    }
    
    void stop() override {
        for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
            (*it)->cleanup();
        }
        systems_.clear();
    }

private:
    std::vector<std::shared_ptr<ISystem>> systems_;
};
```

### Example: Client Scheduler with Variable FPS

```cpp
class ClientScheduler : public IScheduler {
public:
    void addSystem(std::shared_ptr<ISystem> system) override {
        // Same as server
        systems_.push_back(system);
        system->initialize();
    }
    
    void update(Registry& registry, float deltaTime) override {
        // Cap deltaTime to prevent huge jumps
        float cappedDelta = std::min(deltaTime, 0.1f);
        
        for (auto& system : systems_) {
            system->update(registry, cappedDelta);
        }
    }
    
    void stop() override {
        for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
            (*it)->cleanup();
        }
        systems_.clear();
    }

private:
    std::vector<std::shared_ptr<ISystem>> systems_;
};
```

---

## **5. System Execution Order**

**Systems execute in registration order**, which is critical for correctness.

### Server Example

```cpp
ServerScheduler scheduler;

// Authoritative game logic
scheduler.addSystem(std::make_shared<PlayerInputSystem>());    // 1. Process input
scheduler.addSystem(std::make_shared<MonsterAISystem>());      // 2. AI behavior
scheduler.addSystem(std::make_shared<MovementSystem>());       // 3. Apply movement
scheduler.addSystem(std::make_shared<CollisionSystem>());      // 4. Detect collisions
scheduler.addSystem(std::make_shared<DamageSystem>());         // 5. Apply damage
scheduler.addSystem(std::make_shared<DestructionSystem>());    // 6. Remove dead entities
scheduler.addSystem(std::make_shared<SnapshotSystem>());       // 7. Create state snapshot
```

### Client Example

```cpp
ClientScheduler scheduler;

// Prediction, reconciliation, rendering
scheduler.addSystem(std::make_shared<InputSystem>());              // 1. Capture input
scheduler.addSystem(std::make_shared<PredictionSystem>());         // 2. Predict local player
scheduler.addSystem(std::make_shared<SnapshotApplySystem>());      // 3. Apply server snapshot
scheduler.addSystem(std::make_shared<ReconciliationSystem>());     // 4. Reconcile prediction
scheduler.addSystem(std::make_shared<InterpolationSystem>());      // 5. Interpolate remote entities
scheduler.addSystem(std::make_shared<AnimationSystem>());          // 6. Update animations
scheduler.addSystem(std::make_shared<RenderSystem>());             // 7. Draw everything
```

---

## **6. Lifecycle Management**

### Initialization Flow

```cpp
auto system = std::make_shared<MySystem>();
scheduler.addSystem(system);
// → system->initialize() called
```

### Update Flow

```cpp
scheduler.update(registry, deltaTime);
// → For each system in order:
//    system->update(registry, deltaTime)
```

### Cleanup Flow

```cpp
scheduler.stop();
// → For each system in REVERSE order:
//    system->cleanup()
// → Clear all systems
```

---

## **7. Components: Pure Data**

**Components do NOT use interfaces.** They are simple data structures:

```cpp
// Components = pure data, no inheritance
struct Position {
    float x;
    float y;
};

struct Velocity {
    float dx;
    float dy;
};

struct Health {
    int hp;
    int maxHp;
};
```

**Why no interfaces for components?**
- ❌ Breaks cache efficiency (vtables)
- ❌ Adds unnecessary complexity
- ✅ Components are identical on client and server
- ✅ Performance-critical (tight memory layout)

---

## **8. What Uses Interfaces**

| Element | Interface? | Reason |
|---------|-----------|---------|
| **Systems** | ✅ Yes (`ISystem`) | Different logic for client/server |
| **Scheduler** | ✅ Yes (`IScheduler`) | Different timing models |
| **Components** | ❌ No | Pure data, identical everywhere |
| **Registry** | ❌ No | Shared logic, no variation needed |
| **Events** | ❌ No | Pure data |

---

## **9. Testing Systems**

Systems can be tested independently:

```cpp
TEST(MovementSystemTest, AppliesVelocityToPosition) {
    Registry registry;
    MovementSystem system;
    
    EntityId entity = registry.createEntity();
    registry.emplace<Position>(entity, 0.0f, 0.0f);
    registry.emplace<Velocity>(entity, 10.0f, 5.0f);
    
    system.update(registry, 0.1f);
    
    auto& pos = registry.get<Position>(entity);
    EXPECT_FLOAT_EQ(pos.x, 1.0f);  // 10 * 0.1
    EXPECT_FLOAT_EQ(pos.y, 0.5f);  // 5 * 0.1
}
```

See `tests/shared/scheduler/SchedulerTests.cpp` for scheduler test examples.

---

## **10. Summary**

| Component | Type | Location |
|-----------|------|----------|
| `ISystem` | Pure interface | `shared/include/systems/` |
| `IScheduler` | Pure interface | `shared/include/scheduler/` |
| System implementations | Concrete classes | `client/` or `server/` |
| Scheduler implementations | Concrete classes | `client/` or `server/` |

**Key principles:**
* Interfaces define contracts, not implementations
* Systems execute in registration order
* Client and server create their own implementations
* Components remain simple data structures
* Not thread-safe — use from game loop thread only

---

## **11. Related Documentation**

* [Registry](registry.md) — ECS core for entity and component management
* [Systems Overview](systems-overview.md) — High-level system concepts
* [Components Overview](components-overview.md) — Available component types
* [Event Bus](event-bus.md) — System communication mechanism

---

## **12. Source Files**

**Interfaces:**
* `shared/include/systems/ISystem.hpp`
* `shared/include/scheduler/IScheduler.hpp`

**Tests:**
* `tests/shared/scheduler/SchedulerTests.cpp`

**Example implementations:**
* `client/include/systems/AnimationSystem.hpp` — Implements `ISystem`


The **Scheduler** is the orchestrator of the ECS game loop, responsible for executing systems in a deterministic order each frame. This page explains the scheduler interfaces, implementation, and how to use them in both client and server contexts.

---

## **1. Overview**

The scheduler architecture consists of three components:

* **`ISystem`** — Interface that all systems must implement
* **`IScheduler`** — Interface for scheduler implementations
* **`BaseScheduler`** — Concrete implementation suitable for most use cases

This design allows:
* ✅ **Code reuse** between client and server
* ✅ **Consistent system execution model**
* ✅ **Easy extension** for custom scheduler behavior
* ✅ **Testability** through dependency injection

---

## **2. The ISystem Interface**

Located in `shared/include/systems/ISystem.hpp`

### Purpose

`ISystem` defines the contract that all systems must follow. It provides:
* Lifecycle hooks (`onInit`, `onShutdown`)
* Main update loop (`update`)
* Consistent interface for the scheduler

### Interface Definition

```cpp
class ISystem {
public:
    virtual ~ISystem() = default;
    
    virtual void onInit() {}
    virtual void update(Registry& registry, float deltaTime) = 0;
    virtual void onShutdown() {}
};
```

### Methods

| Method | Required | Purpose |
|--------|----------|---------|
| `onInit()` | Optional | Called once when system is registered |
| `update()` | **Required** | Called every frame with registry and deltaTime |
| `onShutdown()` | Optional | Called when scheduler shuts down |

### Creating a System

```cpp
#include "systems/ISystem.hpp"

class MovementSystem : public ISystem {
public:
    void update(Registry& registry, float deltaTime) override {
        // Iterate over entities with Position and Velocity components
        for (EntityId entity : registry.view<Position, Velocity>()) {
            auto& pos = registry.get<Position>(entity);
            auto& vel = registry.get<Velocity>(entity);
            
            // Apply velocity to position
            pos.x += vel.dx * deltaTime;
            pos.y += vel.dy * deltaTime;
        }
    }
};
```

### System Guidelines

**Stateless when possible:**
```cpp
// Good: Operates only on component data
class CollisionSystem : public ISystem {
    void update(Registry& registry, float deltaTime) override {
        // Check collisions using components
    }
};
```

**Stateful when necessary:**
```cpp
// Acceptable: Maintains configuration or timers
class SpawnSystem : public ISystem {
private:
    float spawnTimer_ = 0.0f;
    float spawnInterval_ = 2.0f;
    
public:
    void update(Registry& registry, float deltaTime) override {
        spawnTimer_ += deltaTime;
        if (spawnTimer_ >= spawnInterval_) {
            // Spawn enemy
            spawnTimer_ = 0.0f;
        }
    }
};
```

---

## **3. The IScheduler Interface**

Located in `shared/include/scheduler/IScheduler.hpp`

### Purpose

`IScheduler` defines the contract for scheduler implementations. Different schedulers can have different characteristics while maintaining a consistent API.

### Interface Definition

```cpp
class IScheduler {
public:
    virtual ~IScheduler() = default;
    
    virtual void registerSystem(std::shared_ptr<ISystem> system) = 0;
    virtual void update(Registry& registry, float deltaTime) = 0;
    virtual void shutdown() = 0;
};
```

### Why an Interface?

Different game loop requirements:

| Context | Scheduler Type | Characteristics |
|---------|---------------|-----------------|
| **Server** | `ServerScheduler` | Fixed 60 Hz, deterministic, no rendering |
| **Client** | `ClientScheduler` | Variable FPS, prediction, interpolation, rendering |
| **Testing** | `MockScheduler` | Controllable, step-by-step execution |

---

## **4. BaseScheduler Implementation**

Located in:
* `shared/include/scheduler/BaseScheduler.hpp`
* `shared/src/scheduler/BaseScheduler.cpp`

### Purpose

`BaseScheduler` provides a ready-to-use implementation suitable for both client and server. It can be used directly or inherited for custom behavior.

### Features

* ✅ Sequential system execution in registration order
* ✅ Automatic lifecycle management (init/shutdown)
* ✅ Simple, deterministic execution model
* ✅ No threading or timing logic (left to the game loop)

### Basic Usage

```cpp
#include "scheduler/BaseScheduler.hpp"

int main() {
    Registry registry;
    BaseScheduler scheduler;
    
    // Register systems in execution order
    scheduler.registerSystem(std::make_shared<InputSystem>());
    scheduler.registerSystem(std::make_shared<MovementSystem>());
    scheduler.registerSystem(std::make_shared<RenderSystem>());
    
    // Game loop
    bool running = true;
    while (running) {
        float deltaTime = 0.016f; // 60 FPS
        scheduler.update(registry, deltaTime);
    }
    
    scheduler.shutdown();
    return 0;
}
```

### System Execution Order

**Systems execute in registration order**, which is critical for correctness:

#### Server Example

```cpp
// Server systems: authoritative logic
scheduler.registerSystem(std::make_shared<PlayerInputSystem>());    // 1. Process input
scheduler.registerSystem(std::make_shared<MonsterAISystem>());      // 2. AI behavior
scheduler.registerSystem(std::make_shared<MovementSystem>());       // 3. Apply movement
scheduler.registerSystem(std::make_shared<CollisionSystem>());      // 4. Detect collisions
scheduler.registerSystem(std::make_shared<DamageSystem>());         // 5. Apply damage
scheduler.registerSystem(std::make_shared<DestructionSystem>());    // 6. Remove dead entities
scheduler.registerSystem(std::make_shared<SnapshotSystem>());       // 7. Create state snapshot
```

#### Client Example

```cpp
// Client systems: prediction, reconciliation, rendering
scheduler.registerSystem(std::make_shared<InputSystem>());              // 1. Capture input
scheduler.registerSystem(std::make_shared<PredictionSystem>());         // 2. Predict local player
scheduler.registerSystem(std::make_shared<SnapshotApplySystem>());      // 3. Apply server snapshot
scheduler.registerSystem(std::make_shared<ReconciliationSystem>());     // 4. Reconcile prediction
scheduler.registerSystem(std::make_shared<InterpolationSystem>());      // 5. Interpolate remote entities
scheduler.registerSystem(std::make_shared<AnimationSystem>());          // 6. Update animations
scheduler.registerSystem(std::make_shared<RenderSystem>());             // 7. Draw everything
```

---

## **5. Lifecycle Management**

The scheduler manages system lifecycle automatically:

### Initialization Flow

```cpp
auto system = std::make_shared<MySystem>();
scheduler.registerSystem(system);
// → system->onInit() called immediately
```

### Update Flow

```cpp
scheduler.update(registry, deltaTime);
// → For each system in order:
//    system->update(registry, deltaTime)
```

### Shutdown Flow

```cpp
scheduler.shutdown();
// → For each system in REVERSE order:
//    system->onShutdown()
// → Clear all systems
```

Reverse shutdown order ensures dependent systems clean up correctly.

---

## **6. Advanced: Custom Schedulers**

You can inherit from `BaseScheduler` or implement `IScheduler` directly:

### Example: Fixed-Timestep Server Scheduler

```cpp
class ServerScheduler : public BaseScheduler {
private:
    static constexpr float TICK_RATE = 1.0f / 60.0f; // 60 Hz
    float accumulator_ = 0.0f;
    
public:
    void runFrame(Registry& registry, float realDeltaTime) {
        accumulator_ += realDeltaTime;
        
        // Fixed timestep updates
        while (accumulator_ >= TICK_RATE) {
            update(registry, TICK_RATE);
            accumulator_ -= TICK_RATE;
        }
    }
};
```

### Example: Variable-FPS Client Scheduler

```cpp
class ClientScheduler : public BaseScheduler {
public:
    void runFrame(Registry& registry, float deltaTime) {
        // Cap deltaTime to prevent huge jumps
        float cappedDelta = std::min(deltaTime, 0.1f);
        update(registry, cappedDelta);
    }
};
```

---

## **7. Thread Safety**

⚠️ **BaseScheduler is NOT thread-safe**

* Only access from the game loop thread
* Network threads should use thread-safe queues to communicate with the game loop
* The Registry is also not thread-safe

**Correct pattern:**
```
[Receive Thread] → [Thread-Safe Queue] → [Game Loop] → [Scheduler] → [Systems]
```

---

## **8. Testing Systems**

Systems can be tested independently:

```cpp
TEST(MovementSystemTest, AppliesVelocityToPosition) {
    Registry registry;
    MovementSystem system;
    
    // Create test entity
    EntityId entity = registry.createEntity();
    registry.emplace<Position>(entity, 0.0f, 0.0f);
    registry.emplace<Velocity>(entity, 10.0f, 5.0f);
    
    // Run system
    system.update(registry, 0.1f); // 100ms
    
    // Check results
    auto& pos = registry.get<Position>(entity);
    EXPECT_FLOAT_EQ(pos.x, 1.0f);  // 10 * 0.1
    EXPECT_FLOAT_EQ(pos.y, 0.5f);  // 5 * 0.1
}
```

See `tests/shared/scheduler/SchedulerTests.cpp` for scheduler test examples.

---

## **9. Performance Considerations**

### System Count

* Keep system count reasonable (< 20 systems is typical)
* Each system adds overhead per frame
* Combine related logic when appropriate

### System Complexity

* **Fast path:** Simple queries and component updates
* **Slow path:** Complex algorithms, external I/O

Profile your systems to find bottlenecks.

### View Performance

Systems use views for iteration:
```cpp
registry.view<Position, Velocity>()  // Fast: bitwise signature matching
```

Views are extremely fast due to:
* Sparse-set dense array iteration
* Signature-based filtering
* No virtual calls

---

## **10. Summary**

| Component | Location | Purpose |
|-----------|----------|---------|
| `ISystem` | `shared/include/systems/` | Interface for all systems |
| `IScheduler` | `shared/include/scheduler/` | Interface for schedulers |
| `BaseScheduler` | `shared/include/scheduler/` + `shared/src/scheduler/` | Concrete scheduler implementation |

**Key principles:**
* Systems execute in registration order
* Lifecycle is managed automatically
* Client and server use the same architecture
* Not thread-safe — use from game loop thread only

---

## **11. Related Documentation**

* [Registry](registry.md) — ECS core for entity and component management
* [Systems Overview](systems-overview.md) — High-level system concepts
* [Components Overview](components-overview.md) — Available component types
* [Event Bus](event-bus.md) — System communication mechanism

---

## **12. Source Files**

**Interfaces:**
* `shared/include/systems/ISystem.hpp`
* `shared/include/scheduler/IScheduler.hpp`

**Implementation:**
* `shared/include/scheduler/BaseScheduler.hpp`
* `shared/src/scheduler/BaseScheduler.cpp`

**Tests:**
* `tests/shared/scheduler/SchedulerTests.cpp`

**Example usage:**
* `client/include/systems/AnimationSystem.hpp` — Implements `ISystem`
