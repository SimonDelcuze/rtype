# Introduction

Welcome to the official documentation for our R-Type project.\
This GitBook provides a complete technical overview of the engine, networking model, ECS architecture, server simulation, and client rendering pipeline.

The goal of this documentation is to offer a clear, structured, and exhaustive reference for:

* developers continuing the project
* contributors reading or modifying the codebase
* students learning from the architecture
* anyone needing insight into the gameplay, networking, and ECS design choices

This documentation covers both sides of the project:

#### **Server**

* authoritative gameplay simulation
* deterministic ECS logic
* delta-state snapshot generation
* multithreaded networking

#### **Client**

* interpolation and prediction
* replicated ECS state
* rendering pipeline
* networking threads

#### **Shared Engine**

* custom ECS framework
* registry and views
* shared components
* concurrency utilities

The project is designed around clear separation of responsibilities, deterministic simulation, and high-performance client rendering.\
Each section of this documentation guides you through a different layer of the engine, from the high-level concepts down to the implementation details.
