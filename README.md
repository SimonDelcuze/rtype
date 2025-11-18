# R-Type (WIP)

## Prerequisites
- C++ compiler (GCC/Clang on Linux, MSVC on Windows)
- CMake ≥ 3.16
- Git (used by CMake FetchContent to grab SFML)

No external package manager required; SFML is fetched and built via CMake’s FetchContent during configuration.

## Build
```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
# Build
cmake --build build
# Tests (if enabled)
ctest --test-dir build --output-on-failure
```

## Notes
- Runtime binaries are in `build/` (or per your CMake config), with CMake runtime dir set to repo root.
- Dependencies stay within the build tree; no system-wide install needed (aside des libs bas niveau OS).
