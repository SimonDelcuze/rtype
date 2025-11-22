# Components Overview

## **What Is a Component?**

In our ECS (Entity–Component–System) architecture, a **Component** is a **pure data container** attached to an entity.\
It contains **no behavior**, **no methods**, and **no game logic**.\
A component describes _what an entity has_, while systems describe _what an entity does_.

This strict separation of data and logic is the foundation of the engine’s architecture.

***

## **1. Components Are Pure Data**

A component is simply a struct:

* No inheritance
* No virtual methods
* No game logic
* No polymorphism
* No pointers to other entities

Example pattern:

```cpp
struct TransformComponent {
    float x;
    float y;
    float rotation;
};
```

This struct does **not** know how to move, how to draw itself, or how to interact with other entities.\
It simply stores data that other systems will use.

***

## **2. Components Define an Entity’s Identity**

Entities are empty numeric IDs.\
Components give them meaning.

Example:

| Entity | Components                  | Meaning             |
| ------ | --------------------------- | ------------------- |
| 12     | Transform + Velocity        | A moving object     |
| 45     | Transform + Sprite          | A visual object     |
| 99     | Transform + Hitbox + Health | A damageable entity |

Components are like “tags” or “properties” that accumulate to form complex entities.

***

## **3. Components Are Stored in Fast, Cache-Friendly Arrays**

Components are stored using a **sparse/dense array model**, giving:

* O(1) insertion
* O(1) removal
* O(1) lookup
* Zero fragmentation
* High cache locality
* Very fast iteration in systems

This is what makes ECS ideal for:

* Real-time servers (deterministic 60 FPS loop)
* Clients rendering high numbers of entities
* Multiplayer replication (clean separation of data)

***

## **4. Components Do Not Interact with Each Other**

Components **never** reference:

* other components
* other entities
* systems
* engine state

They are **independent data units**.\
This ensures:

* No hidden dependencies
* No cyclic logic
* No “God object” problems
* Easy replication over the network
* Perfect separation between server and client data

***

## **5. Components Are Different on the Server and Client**

Even though they share the same ECS framework, each side uses different sets of components:

#### **Server components**

Describe **gameplay state**, such as:

* movement
* health
* collisions
* AI
* missile behavior

#### **Client components**

Describe **visual state**, such as:

* sprite
* animation
* interpolation buffers
* rendering attributes

This split keeps:

* the server _authoritative and minimal_
* the client _responsive and visually rich_

***

## **6. Components Enable Modular Game Logic**

Because logic is entirely in systems, adding a new feature is easy:

→ Add a new component type\
→ Add a new system that uses it

No need to touch existing code.

This makes the engine:

* flexible
* clean
* easy to maintain
* easy to extend
* ideal for complex gameplay or networking

***

## **7. Why Components Matter for Networking**

Because components are plain data:

* They can be serialized directly
* They can be delta-compressed
* They can be transmitted over UDP efficiently
* They can be reconstructed on the client easily

This pattern dramatically simplifies:

* replication
* prediction
* interpolation
* reconciliation

***

## **Summary**

A **Component** is:

✔ A simple data struct\
✔ No logic\
✔ Gives meaning to an entity\
✔ Stored in fast arrays\
✔ Used by systems to perform logic\
✔ Different between server & client\
✔ Easy to replicate over network\
✔ The foundation of the ECS engine

Components represent **the data model** of the entire game.
