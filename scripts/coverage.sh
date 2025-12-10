#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-coverage"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="--coverage" \
      -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
      -DCMAKE_SHARED_LINKER_FLAGS="--coverage" \
      -DBUILD_TESTS=ON \
      ..

cmake --build . -- -j"$(nproc)"
ctest --output-on-failure

if command -v gcovr >/dev/null 2>&1; then
    gcovr -r "$ROOT_DIR" "$BUILD_DIR" \
          --filter "$ROOT_DIR/client" \
          --filter "$ROOT_DIR/server" \
          --filter "$ROOT_DIR/shared" \
          --exclude ".*/include/.*" \
          --exclude ".*_deps.*" \
          --exclude ".*tests.*" \
          --exclude ".*build-coverage.*" \
          --print-summary
else
    echo "gcovr not found; install it to print coverage summary."
fi
