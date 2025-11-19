#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ "$#" -eq 0 ]; then
    TARGETS=("client" "server" "shared" "tests")
else
    TARGETS=("$@")
fi

PATHS=()
for target in "${TARGETS[@]}"; do
    case "$target" in
        client|server|shared|tests)
            PATHS+=("$PROJECT_ROOT/$target")
            ;;
        *)
            echo "[format.sh] Unknown target '$target' (allowed: client, server, shared, tests)" >&2
            exit 1
            ;;
    esac
done

files=$(rg --files -g '*.{cpp,hpp}' "${PATHS[@]}")

if [ -z "$files" ]; then
    echo "[format.sh] No file to format."
    exit 0
fi

echo "[format.sh] Starting formatting..."
clang-format -i $files
echo "[format.sh] Formatting completed."
