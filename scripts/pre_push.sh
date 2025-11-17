#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

check_files=$(rg --files -g '*.{cpp,hpp}' "$PROJECT_ROOT/client" "$PROJECT_ROOT/server" "$PROJECT_ROOT/shared" "$PROJECT_ROOT/tests")

if [ -n "$check_files" ]; then
    echo "[pre_pr] format check"
    clang-format --dry-run --Werror $check_files
fi

echo "[pre_pr] lint"
"$PROJECT_ROOT/scripts/lint.sh"

echo "[pre_pr] tests"
"$PROJECT_ROOT/scripts/run_tests.sh"
