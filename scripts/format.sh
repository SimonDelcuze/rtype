#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

files=$(rg --files -g '*.{cpp,hpp}' "$PROJECT_ROOT/client" "$PROJECT_ROOT/server" "$PROJECT_ROOT/shared" "$PROJECT_ROOT/tests")

if [ -z "$files" ]; then
    echo "[format.sh] No file to format."
    exit 0
fi

echo "[format.sh] Starting formatting..."
clang-format -i $files
echo "[format.sh] Formatting completed."
