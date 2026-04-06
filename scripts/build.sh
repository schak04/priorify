#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ROOT_DIR="$SCRIPT_DIR/.."
SRC_DIR="$ROOT_DIR/src"
LIB_DIR="$ROOT_DIR/lib"
BUILD_DIR="$ROOT_DIR/build"
BIN_DIR="$ROOT_DIR/bin"
DATA_DIR="$ROOT_DIR/data"

mkdir -p "$BUILD_DIR" "$BIN_DIR" "$DATA_DIR"

echo "Compiling SQLite source..."
gcc -c "$LIB_DIR/sqlite3.c" -o "$BUILD_DIR/sqlite3.o"

echo "Compiling C++ sources..."
g++ -c "$SRC_DIR/db.cpp" -o "$BUILD_DIR/db.o"
g++ -c "$SRC_DIR/task_manager.cpp" -o "$BUILD_DIR/task_manager.o"
g++ -c "$SRC_DIR/ui.cpp" -o "$BUILD_DIR/ui.o"
g++ -c "$SRC_DIR/main.cpp" -o "$BUILD_DIR/main.o"

echo "Linking to create executable..."
g++ "$BUILD_DIR/main.o" "$BUILD_DIR/ui.o" "$BUILD_DIR/task_manager.o" "$BUILD_DIR/db.o" "$BUILD_DIR/sqlite3.o" -o "$BIN_DIR/priorify" -lpthread -ldl -O2

echo
echo "Build complete. Run with: $BIN_DIR/priorify"