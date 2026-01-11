# 🌌 The Ultimate R-Type Contribution Guide (A to Z)

Welcome, Pilot! 🚀 We are thrilled that you want to contribute to the R-Type project. Whether you're fixing a bug, adding a new enemy, or improving the network protocol, this guide will provide you with everything you need to know to get started and succeed.

---

## 🗺️ Table of Contents

1.  [🏗️ Project Architecture](#-project-architecture)
2.  [💻 Setting Up Your Environment](#-setting-up-your-environment)
3.  [🚀 Your First Contribution: Step-by-Step](#-your-first-contribution-step-by-step)
4.  [🛠️ How to Add a New Feature (ECS Tutorial)](#-how-to-add-a-new-feature-ecs-tutorial)
5.  [🎨 Coding Standards & Style](#-coding-standards--style)
6.  [🧪 Testing & Quality Assurance](#-testing--quality-assurance)
7.  [📚 Documentation Requirements](#-documentation-requirements)
8.  [⚙️ Continuous Integration (CI)](#-continuous-integration-ci)
9.  [🚩 Pull Request Workflow](#-pull-request-workflow)
10. [🔍 Debugging Tips](#-debugging-tips)

---

## 🏗️ Project Architecture

Before diving into the code, it's essential to understand how the project is structured.

```text
.
├── client/             # 🕹️ Client-side source code (SFML, UI, Prediction)
├── server/             # 🖥️ Server-side source code (Lobby, Game Instances)
├── shared/             # 📦 Common code (ECS Engine, Network Protocols, Components)
├── docs/               # 📚 GitBook documentation
├── tests/              # 🧪 Unit and Integration tests
├── assets/             # 🎨 Game assets (Textures, Fonts, Levels)
├── scripts/            # 🛠️ Utility scripts (Linting, Build, CI)
└── CMakeLists.txt      # ⚙️ Build system configuration
```

### The Core: Entity Component System (ECS)
Our engine is data-driven.
-   **Entities**: Just an ID.
-   **Components**: Pure data (structs).
-   **Systems**: Logic that processes entities with specific components.

---

## 💻 Setting Up Your Environment

### 1. Prerequisites
-   **Compiler**: C++20 support (GCC 11+, Clang 13+, MSVC 2022+).
-   **Build Tool**: CMake 3.16+ and Ninja (recommended).
-   **Dependencies**: SFML 3.0, nlohmann-json, GoogleTest (all handled via CPM).

### 2. OS Specific Dependencies (Linux)
```bash
sudo apt-get update
sudo apt-get install -y cmake ninja-build g++ libx11-dev libxcursor-dev \
    libxi-dev libxrandr-dev libxinerama-dev libudev-dev libgl1-mesa-dev \
    libopenal-dev libsndfile1-dev libfreetype6-dev libjpeg-dev
```

### 3. Building the Project
```bash
# Configure
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Build
cmake --build build

# Run
./build/r-type_server
./build/r-type_client
```

---

## � Your First Contribution: Step-by-Step

1.  **Find an Issue**: Look for "good first issue" labels on GitHub.
2.  **Fork & Clone**: Create your own copy of the repo.
3.  **Branch**: `git checkout -b feature/my-new-feature`.
4.  **Code**: Implement your changes (follow the ECS tutorial below!).
5.  **Test**: Ensure `ctest` passes.
6.  **Format**: Run `make format`.
7.  **Push & PR**: Submit your Pull Request for review.

---

## 🛠️ How to Add a New Feature (ECS Tutorial)

Let's say you want to add a **"Poison"** effect that deals damage over time.

### Step 1: Create the Component
In `shared/include/components/PoisonComponent.hpp`:
```cpp
struct PoisonComponent {
    float damagePerSecond;
    float remainingDuration;
    
    static PoisonComponent create(float dps, float duration) {
        return {dps, duration};
    }
};
```

### Step 2: Create the System
In `server/src/systems/PoisonSystem.cpp`:
```cpp
void PoisonSystem::update(Registry& registry, float dt) {
    auto& healths = registry.getComponents<HealthComponent>();
    auto& poisons = registry.getComponents<PoisonComponent>();

    for (auto [entity, health, poison] : Zip(healths, poisons)) {
        health.hp -= poison.damagePerSecond * dt;
        poison.remainingDuration -= dt;
        
        if (poison.remainingDuration <= 0) {
            registry.removeComponent<PoisonComponent>(entity);
        }
    }
}
```

### Step 3: Register the System
In the Game Loop initialization:
```cpp
registry.addSystem<PoisonSystem>();
```

---

## 🎨 Coding Standards & Style

We use **Clang-Format** and **Clang-Tidy** to keep the code clean.

### Naming Conventions
-   **Classes/Structs**: `PascalCase` (e.g., `EntityManager`)
-   **Functions/Methods**: `camelCase` (e.g., `createEntity()`)
-   **Variables**: `camelCase` (e.g., `entityCount`)
-   **Private Members**: `camelCase_` (with trailing underscore, e.g., `registry_`)
-   **Constants**: `kPascalCase` (e.g., `kMaxPlayers`)

### Modern C++ Practices
-   Use `auto` when the type is obvious.
-   Prefer `std::unique_ptr` over raw pointers.
-   Use `std::string_view` for read-only string parameters.
-   Always use `override` for virtual functions.

---

## 🧪 Testing & Quality Assurance

**Tests are mandatory.** If it's not tested, it's broken.

### Writing a Unit Test (GoogleTest)
In `tests/shared/test_PoisonSystem.cpp`:
```cpp
TEST(PoisonSystem, DealsDamage) {
    Registry registry;
    auto entity = registry.createEntity();
    registry.addComponent(entity, HealthComponent{100.0f});
    registry.addComponent(entity, PoisonComponent{10.0f, 5.0f});

    PoisonSystem system;
    system.update(registry, 1.0f);

    EXPECT_EQ(registry.getComponent<HealthComponent>(entity).hp, 90.0f);
}
```

---

## 📚 Documentation Requirements

Every new feature must be documented in the `docs/` folder.

1.  **Component Doc**: Create `docs/client/components/my-component.md`.
2.  **System Doc**: Create `docs/client/systems/my-system.md`.
3.  **Update Summary**: Add your new pages to `docs/SUMMARY.md`.

---

## ⚙️ Continuous Integration (CI)

Our CI pipeline (GitHub Actions) runs on every PR:
-   **Format**: Checks if you ran `make format`.
-   **Lint**: Runs `clang-tidy` for deep code analysis.
-   **Build**: Compiles on Linux and Windows.
-   **Test**: Runs all unit and integration tests.

---

## 🚩 Pull Request Workflow

-   **Title**: Use [Conventional Commits](https://www.conventionalcommits.org/) (e.g., `feat: add poison system`).
-   **Description**: Explain *what* you changed and *why*.
-   **Screenshots**: If you changed the UI, add a screenshot or GIF.
-   **Review**: At least one maintainer must approve your PR.

---

## 🔍 Debugging Tips

-   **Logging**: Use our internal logger: `LOG_INFO("Entity {} spawned", id);`.
-   **GDB/LLDB**: Use a debugger! Our CMake setup includes debug symbols in `Debug` mode.
-   **Sanitizers**: Build with `-DENABLE_SANITIZERS=ON` to catch memory leaks and thread races.

---

<p align="center">
  <b>Hope you enjoyed this How-to-Contributing! If you have any questions, feel free to ask on GitHub.</b>
</p>
