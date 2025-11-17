#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="$PROJECT_ROOT/build"

usage() {
    echo "Usage: $0 [all|client|server|shared]"
    echo "  all (default): build all targets"
    echo "  client       : build only rtype_client"
    echo "  server       : build only rtype_server"
    echo "  shared       : build only rtype_shared_tests"
    exit 1
}

TARGET="${1-all}"

if [ ! -d "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "[build.sh] Configuring CMake in '$BUILD_DIR'..."
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DBUILD_TESTS=ON
fi

CMAKE_TARGET_ARG=""
case "$TARGET" in
    all|build)
        echo "[build.sh] Building all targets..."
        ;;
    client)
        echo "[build.sh] Building client target..."
        CMAKE_TARGET_ARG="--target rtype_client"
        ;;
    server)
        echo "[build.sh] Building server target..."
        CMAKE_TARGET_ARG="--target rtype_server"
        ;;
    shared)
        echo "[build.sh] Building shared tests target..."
        CMAKE_TARGET_ARG="--target rtype_shared_tests"
        ;;
    *)
        usage
        ;;
esac

cmake --build "$BUILD_DIR" $CMAKE_TARGET_ARG -j
