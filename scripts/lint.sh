#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "[lint.sh] Generating compile_commands.json..."
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DBUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

BASE_REF="${LINT_BASE_REF:-origin/main}"
changed_files=$(git diff --name-only "${BASE_REF}"...HEAD -- '*.cpp' '*.hpp')

if [ -z "$changed_files" ]; then
    echo "[lint.sh] No changed C++ files to lint (base=${BASE_REF})."
    exit 0
fi

clang-tidy -p "$BUILD_DIR" --header-filter='^(client|server|shared)/' --extra-arg=-w --quiet $changed_files
