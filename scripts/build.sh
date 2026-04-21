#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR/.."
BUILD_DIR="$ROOT_DIR/build"

echo "Configuring project with CMake..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR"

echo "Building project..."
cmake --build "$BUILD_DIR"

echo
echo "Build complete. Run with: $ROOT_DIR/bin/priorify"