# R-Type (WIP)

## Prerequisites
- C++ compiler (GCC/Clang on Linux, MSVC on Windows)
- CMake ≥ 3.16
- Git (used by CPM.cmake to grab SFML/nlohmann/json/googletest)

Dependencies are handled with CPM.cmake (CMake package manager); they are fetched and built during configuration, no system-wide installs needed.

## Build

### Option 1: CMake (Cross-platform)
```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
# Build
cmake --build build
# Tests
ctest --test-dir build --output-on-failure
```

### Option 2: Makefile (Linux/macOS)
```bash
make              # Build all targets
make client       # Build client only
make server       # Build server only
make tests        # Run all tests
make clean        # Remove build directory
make re           # Clean and rebuild
make fclean       # Full clean (binaries + build artifacts)
```

### Option 3: PowerShell (Windows)
```powershell
.\scripts\build.ps1                   # Build all targets
.\scripts\build.ps1 -Target client    # Build client only
.\scripts\build.ps1 -Target server    # Build server only
```

## Notes
- Runtime binaries are in `build/` (or per your CMake config), with CMake runtime dir set to repo root.
- Dependencies stay within the build tree; no system-wide install needed (except for low-level OS libraries).
- Optionally set `CPM_SOURCE_CACHE` (env var) or `-DCPM_SOURCE_CACHE=/path/cache` to avoid re-downloading dependencies in CI/CD
