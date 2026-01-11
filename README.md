<p align="center">
  <img src="assets/logo.png" alt="R-Type Logo" width="400">
</p>

<p align="center">
  <b>A high-performance, multiplayer space shooter engine built with C++20 and custom ECS.</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-Latest-blue.svg?style=for-the-badge&logo=c%2B%2B" alt="C++">
  <img src="https://img.shields.io/badge/SFML-3.0-green.svg?style=for-the-badge&logo=sfml" alt="SFML 3.0">
  <img src="https://img.shields.io/badge/Platform-Cross--Platform-lightgrey.svg?style=for-the-badge" alt="Platform">
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge" alt="License">
</p>

---

![R-Type Banner](assets/banner.png)

## 🌌 Overview

**R-Type** is a modern reimagining of the classic arcade shooter, developed as a robust multiplayer engine. It features a custom-built **Entity Component System (ECS)**, a high-performance network protocol, and a scalable server architecture capable of handling multiple concurrent game instances.

### ✨ Key Features

- 🚀 **Custom ECS Engine**: Ultra-fast entity management with cache-friendly data structures.
- 🌐 **Multiplayer Synchronization**: Advanced client-side prediction and server reconciliation for lag-free gameplay.
- 🛠️ **Level Editor**: Create and share custom levels with a dedicated visual tool.
- 🛡️ **Secure Authentication**: Integrated lobby system with user accounts, statistics, and secure login.
- 🎨 **Dynamic Rendering**: Smooth animations, parallax scrolling, and high-fidelity visual effects powered by SFML 3.0.
- 📊 **Scalable Architecture**: Multi-threaded server design capable of hosting hundreds of players.

---

## 📸 Screenshots

<p align="center">
  <i>[ PLACEHOLDER: Insert Gameplay Screenshot 1 ]</i>
  <br>
  <i>[ PLACEHOLDER: Insert Level Editor Screenshot ]</i>
  <br>
  <i>[ PLACEHOLDER: Insert Lobby Menu Screenshot ]</i>
</p>

---

## 🏗️ The ECS Architecture

Our engine is built on a custom **Entity Component System (ECS)**. This pattern decouples data (Components) from logic (Systems), allowing for high-performance processing and extreme flexibility.

### How it works:

```mermaid
graph LR
    subgraph "Entities"
        E1[Entity 1]
        E2[Entity 2]
        E3[Entity 3]
    end

    subgraph "Components (Data Only)"
        C1["Transform {x, y}"]
        C2["Velocity {vx, vy}"]
        C3["Sprite {texture_id}"]
        C4["Health {hp}"]
    end

    subgraph "Systems (Logic Only)"
        S1[Movement System]
        S2[Render System]
        S3[Collision System]
    end

    E1 --- C1
    E1 --- C3
    
    E2 --- C1
    E2 --- C2
    E2 --- C3
    
    E3 --- C1
    E3 --- C4

    S1 -->|Processes| C1
    S1 -->|Processes| C2
    S2 -->|Processes| C1
    S2 -->|Processes| C3
    S3 -->|Processes| C1
    S3 -->|Processes| C4
```

- **Entities**: Simple unique IDs representing game objects (Player, Enemy, Bullet).
- **Components**: Pure data structures (POD) attached to entities. They define *what* an entity is.
- **Systems**: Logic units that run every frame. They query entities that have a specific set of components and update them. For example, the **Movement System** only cares about entities with both `Transform` and `Velocity`.

---

## 📚 Documentation

Our documentation is extensive and organized for developers of all levels.

| Section | Description |
| :--- | :--- |
| 🚀 [**Installation**](docs/installation/README.md) | How to build and run the project on Linux, macOS, and Windows. |
| 🏗️ [**Architecture**](docs/architecture/README.md) | Deep dive into the ECS, Core Components, and Engine design. |
| 🌐 [**Network**](docs/network/README.md) | Detailed protocol specifications, authentication, and security. |
| 🖥️ [**Server**](docs/server/README.md) | Server-side logic, thread management, and level systems. |
| 🕹️ [**Client**](docs/client/README.md) | Rendering pipeline, UI module, and client-side prediction. |

---

## 🛠️ Quick Start

### Prerequisites

- **C++20** compatible compiler (GCC 11+, Clang 13+, MSVC 2022+)
- **CMake** 3.16+
- **Git**

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/SimonDelcuze/rtype.git
cd rtype

# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run the server
./r-type_server

# Run the client
./r-type_client
```

---

## 🤝 Contributing

Contributions are welcome! Please check our [Contribution Guidelines](CONTRIBUTING.md) and [Code of Conduct](CODE_OF_CONDUCT.md).

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request
