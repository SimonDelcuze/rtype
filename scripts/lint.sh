#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "[lint.sh] Generating compile_commands.json..."
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DBUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

files=$(rg --files -g '*.{cpp,hpp}' "$PROJECT_ROOT/client" "$PROJECT_ROOT/server" "$PROJECT_ROOT/shared" "$PROJECT_ROOT/tests")

if [ -z "$files" ]; then
    echo "[lint.sh] No file to check."
    exit 0
fi

clang-tidy -p "$BUILD_DIR" --header-filter='^(client|server|shared|tests)/' --extra-arg=-w --quiet $files
