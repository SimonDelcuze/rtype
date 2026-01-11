# Installation Guide

## Installation Guide

This guide explains how to download, build, and run the project.\
The repository uses CMake and provides a Makefile wrapper for easily building the client, server, shared library, running tests, formatting and linting.

Project structure (excerpt):

```
.
├── client
├── server
├── shared
├── scripts
├── Makefile
├── CMakeLists.txt
...
```

---

### 1. Requirements

Make sure you have installed:

- A C++20-capable compiler (e.g., GCC 11+, Clang 13+, MSVC 19.30+)
- CMake version ≥ 3.20
- Git
- Make (or an equivalent build tool)
- On supported OSes: SFML will be fetched automatically by CMake, so you do not have to install it manually.

---

### 2. Cloning the Repository

```bash
git clone https://github.com/SimonDelcuze/rtype.git
cd rtype
```

---

### 3. Building the Project

#### A. Using the Makefile (recommended)

The root directory contains a `Makefile` with convenient targets:

```bash
make            # or `make all`: runs build for client, server, shared
make client     # builds only the client target
make server     # builds only the server target

make tests       # runs all test suites
make test_client # runs only client tests
make test_server # runs only server tests
make test_shared # runs only shared library tests

make format      # formats all code
make format_client # formats client + shared
make format_server # formats server + shared
make lint        # runs lint checks

make clean      # removes build directory
make fclean     # removes binaries & build artifacts
make re         # full rebuild (fclean + all)
```

Running `make` will automatically invoke the `scripts/build.sh` script and configure/build the project.

#### B. Manual CMake Build (optional)

If you prefer more control:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

This will generate the executables:

- `r-type_server`
- `r-type_client`

---

### 4. Running the Server

**With Makefile:**

```bash
make server
./r-type_server
```

Or, from the build directory:

```bash
./r-type_server
```

---

### 5. Running the Client

**With Makefile:**

```bash
make client
./r-type_client
```

Ensure you run the client from the build root (or set the working directory correctly), so that asset paths are correct.\
Assets are located under:

```
client/assets/backgrounds
client/assets/sprites
```

---

### 6. Running the Test Suite

```bash
make tests
```

Or to run specific targets:

```bash
make test_client
make test_server
make test_shared
```

---

### 7. Code Formatting & Linting

To keep code style consistent:

```bash
make format       # formats full project
make lint         # runs lint checks
```

You can also format subsets:

```bash
make format_client
make format_server
```

---

### 8. Cleaning and Rebuilding

```bash
make clean   # remove build directory
make fclean  # remove binaries and build artifacts
make re      # full rebuild
```

---

### 9. Troubleshooting Tips

- If textures or sprites fail to load, check that you are running the client from the correct working directory (so that asset paths are valid).
- If SFML isn’t found or build fails, delete the build directory and re-run `make` (or the manual CMake steps).
- Ensure your compiler supports C++20 and that CMake is up to date.

---

### 10. Summary

- The project supports two main executables: server and client.
- Shared library contains ECS and common code.
- Use `make` as a convenient wrapper around build operations, tests, linting, and formatting.
- Assets are included in the client folder and must be found at runtime.
- The code is set up for development, testing, and iteration.
