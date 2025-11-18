#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="$PROJECT_ROOT/build"

usage() {
    echo "Usage: $0 [all|client|server|shared]"
    echo "  all (default): run all tests"
    echo "  client       : build client and run client tests"
    echo "  server       : build server and run server tests"
    echo "  shared       : build shared and run shared tests"
    exit 1
}

MODE="${1-all}"

case "$MODE" in
    all)
        echo "[run_tests.sh] Building all via build.sh..."
        "$SCRIPT_DIR/build.sh"
        ;;
    client)
        echo "[run_tests.sh] Building client via build.sh..."
        "$SCRIPT_DIR/build.sh" client
        ;;
    server)
        echo "[run_tests.sh] Building server via build.sh..."
        "$SCRIPT_DIR/build.sh" server
        ;;
    shared)
        echo "[run_tests.sh] Building shared via build.sh..."
        "$SCRIPT_DIR/build.sh" shared
        ;;
    *)
        usage
        ;;
esac

cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DBUILD_TESTS=ON

case "$MODE" in
    all)
        cmake --build "$BUILD_DIR" --target rtype_shared_tests rtype_server_tests rtype_client_tests
        ;;
    client)
        cmake --build "$BUILD_DIR" --target rtype_client_tests
        ;;
    server)
        cmake --build "$BUILD_DIR" --target rtype_server_tests
        ;;
    shared)
        cmake --build "$BUILD_DIR" --target rtype_shared_tests
        ;;
esac

cd "$BUILD_DIR"

case "$MODE" in
    all)
        echo "[run_tests.sh] Running all tests..."
        "$PROJECT_ROOT/rtype_shared_tests" --gtest_color=yes || true
        "$PROJECT_ROOT/rtype_server_tests" --gtest_color=yes || true
        "$PROJECT_ROOT/rtype_client_tests" --gtest_color=yes || true
        ;;
    client)
        echo "[run_tests.sh] Running client tests..."
        "$PROJECT_ROOT/rtype_client_tests" --gtest_color=yes
        ;;
    server)
        echo "[run_tests.sh] Running server tests..."
        "$PROJECT_ROOT/rtype_server_tests" --gtest_color=yes
        ;;
    shared)
        echo "[run_tests.sh] Running shared tests..."
        "$PROJECT_ROOT/rtype_shared_tests" --gtest_color=yes
        ;;
esac
