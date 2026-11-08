#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
RAW_DIR="$SCRIPT_DIR/raw"
BUILD_BIN="$PROJECT_ROOT/build/bench_runner"

mkdir -p "$RAW_DIR"
mkdir -p "$PROJECT_ROOT/build"

echo "Capturing benchmark environment details..."
{
    echo "======================================================================="
    echo "BENCHMARK ENVIRONMENT INFORMATION"
    echo "======================================================================="
    echo "Date: $(date -u)"
    echo "OS: $(uname -srmo)"
    echo "Distribution: $(cat /etc/os-release | grep PRETTY_NAME | cut -d= -f2 | tr -d '\"')"
    echo "Kernel: $(uname -r)"
    echo "Architecture: $(uname -m)"
    echo "CPU Model: $(lscpu | grep 'Model name:' | sed 's/Model name:[ \t]*//')"
    echo "CPU Cores / Threads: $(lscpu | grep 'CPU(s):' | head -n1 | awk '{print $2}')"
    echo "CPU Max Frequency: $(lscpu | grep 'CPU max MHz:' | awk '{print $4}') MHz"
    echo "Compiler: $(g++ --version | head -n1)"
    echo "Standard: C++20 (-std=c++20)"
    echo "Optimization Level: -O3"
    echo "SQLite Version: $(grep '#define SQLITE_VERSION ' "$PROJECT_ROOT/lib/sqlite3.h" | awk '{print $3}' | tr -d '\"')"
    echo "FTXUI Version: v5.0.0"
    echo "Git Commit Baseline: $(git -C "$PROJECT_ROOT" rev-parse HEAD)"
    echo "======================================================================="
} > "$RAW_DIR/environment.txt"

echo "Compiling benchmark harness..."
g++ -std=c++20 -O3 \
    -I"$PROJECT_ROOT/headers" \
    -I"$PROJECT_ROOT/lib" \
    -I"$PROJECT_ROOT/build/_deps/ftxui-src/include" \
    "$SCRIPT_DIR/bench_runner.cpp" \
    "$PROJECT_ROOT/build/libsqlite3.a" \
    "$PROJECT_ROOT/build/_deps/ftxui-build/libftxui-dom.a" \
    "$PROJECT_ROOT/build/_deps/ftxui-build/libftxui-screen.a" \
    -lpthread -ldl \
    -o "$BUILD_BIN"

echo "Running full benchmark suite..."
"$BUILD_BIN" --all > "$RAW_DIR/full_benchmark_run.txt"

echo "Running and splitting individual benchmark raw logs..."
"$BUILD_BIN" --opt1 > "$RAW_DIR/opt1_database_caching.txt"
"$BUILD_BIN" --opt2 > "$RAW_DIR/opt2_keyhint_passing.txt"
"$BUILD_BIN" --opt3a > "$RAW_DIR/opt3_monthnames_vector.txt"
"$BUILD_BIN" --opt3b > "$RAW_DIR/opt3_task_passing.txt"

cat "$RAW_DIR/full_benchmark_run.txt"
echo "Benchmarks completed successfully. Raw results saved to $RAW_DIR"
