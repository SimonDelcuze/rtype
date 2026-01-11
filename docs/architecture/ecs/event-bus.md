# Event Bus

The **Event Bus** is a lightweight communication mechanism used inside the ECS architecture to keep systems **decoupled** while still allowing them to react to important events in the game.

Unlike traditional object-oriented designs, where objects call methods on each other, our ECS uses:

* **Components → data only**
* **Systems → logic only**
* **Event Bus → messaging between systems**

This ensures a clean separation of responsibilities and prevents cross-system dependencies.

***

## **1. Why an Event Bus?**

Systems should not:

* Call each other directly
* Share state
* Know about the internal logic of other systems

However, systems often need to _react_ to what another system did, for example:

* The CollisionSystem detects a hit → the HealthSystem must process damage
* The HealthSystem decides an entity died → the DestructionSystem must remove it
* The Server networking layer receives a new player → the SpawnSystem must spawn the ship

The Event Bus solves this.

It acts as a **safe intermediate layer**.

***

## **2. How It Works**

#### **Systems can:**

* **Emit events**
* **Subscribe to events**
* **Process events each frame**

#### **Events are simple structs** (like components):

```cpp
struct DamageEvent {
    Entity target;
    int amount;
};
```

#### Systems push events:

```cpp
eventBus.emit<DamageEvent>({entity, 10});
```

#### Other systems subscribe:

```cpp
eventBus.subscribe<DamageEvent>([](const DamageEvent& e) {
    // apply damage
});
```

***

## **3. When Are Events Processed?**

Events are not processed immediately when emitted.\
Instead, the Event Bus stores them and processes them during the **Event phase** of the frame.

This avoids problems like:

* Modifying components during iteration
* Destroying an entity inside a system still iterating over it
* Triggering recursive system execution

Frame order:

1. All systems run
2. All events emitted
3. Event Bus processes events
4. Cleanup (entity destruction, component removals, etc.)

This ensures **safe, deterministic behavior**.

***

## **4. Types of Events**

The engine uses events for multiple purposes:

#### ✔ Gameplay Events

* DamageEvent
* EntitySpawnEvent
* EntityDeathEvent
* MonsterShootEvent

#### ✔ Engine/Internal Events

* DestroyEntityEvent
* AddComponentEvent
* RemoveComponentEvent

#### ✔ Networking Events

* ClientConnectedEvent
* ClientDisconnectedEvent
* SnapshotReceivedEvent

#### ✔ Rendering Events (Client only)

* AnimationFinishedEvent
* PlaySoundEvent

These vary by subsystem (server vs client), but the design remains consistent.

***

## **5. Why Not Use Direct System Calls?**

Direct system calls introduce:

* Tight coupling
* Hard-to-maintain dependencies
* Cross-update bugs
* Order-sensitive behavior

The Event Bus prevents:

* Calling logic at the wrong time
* Systems depending on each other’s internal state
* Priority issues (“which system should run first?”)

Instead, everything is synchronized during the controlled **event flush** step.

***

## **6. Benefits for a Multiplayer Game**

In a networked environment, the Event Bus provides:

#### ✔ Clean separation between networking and gameplay

Network events become gameplay events, which become ECS updates.

#### ✔ Easy replication of gameplay events

Example:

* Server emits a `PlayerShotEvent`
* Server spawns missile entity
* Server notifies clients through delta-state or event packets

#### ✔ Robustness

Unexpected client inputs or packet order do not break logic.

#### ✔ Determinism

Events are processed in consistent order every frame.

***

## **7. Summary**

The Event Bus provides:

| Feature          | Benefit                                          |
| ---------------- | ------------------------------------------------ |
| Decoupling       | Systems remain isolated and clean                |
| Safety           | No modification during iteration                 |
| Determinism      | All events processed in fixed phase              |
| Flexibility      | Add gameplay mechanics without modifying systems |
| Networking-ready | Events map cleanly to network messages           |

It is one of the backbone tools that make the ECS scalable, safe, and maintainable.
