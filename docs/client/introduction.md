# Introduction

The client is responsible for rendering the game, handling user input, and ensuring that the player experiences a smooth and visually consistent representation of the authoritative server state.\
Unlike the server, the client does not run authoritative gameplay logic. Instead, it focuses on:

* sending player input to the server
* receiving authoritative snapshots
* predicting the player’s local movement
* interpolating remote entities
* correcting discrepancies with server data
* rendering the current visual state

The client executes a local ECS (Entity–Component–System) pipeline focused on animation, interpolation, and rendering. Gameplay rules are not simulated here; the server remains the unique source of truth.

The client must remain responsive even when network latency or packet loss occur. This is achieved through a combination of local prediction, reconciliation, and interpolation techniques that allow the game to feel smooth despite the inherent unreliability of UDP networking.

This section describes the client’s architecture, threading model, data flow, and main loop.
