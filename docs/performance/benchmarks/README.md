# Performance Benchmarks: Methodology and Results

This directory contains the benchmarking methodology, harness implementation, raw outputs, and environment metadata for the performance optimisations implemented in Priorify.

## 1. Test Environment

All benchmarks were compiled and executed natively in the following environment:

- Operating System: Linux 7.1.3-arch1-3 x86_64 (Arch Linux)
- Kernel: 7.1.3-arch1-3
- Processor: 11th Gen Intel(R) Core(TM) i5-11300H @ 3.10GHz (4 cores, 8 threads, max frequency 4400 MHz)
- Compiler: g++ (GCC) 16.1.1 20260625
- Compilation Flags: `-std=c++20 -O3 -Iheaders -Ilib -Ibuild/_deps/ftxui-src/include`
- SQLite Version: 3.41.2
- FTXUI Version: v5.0.0
- Git Baseline: bce3b0e46cced0e04c736d825261be7d275c6784

The complete captured environment output is preserved in `docs/performance/benchmarks/raw/environment.txt`.

## 2. Benchmarking Architecture and Methodology

To obtain defensible and reproducible measurements without modifying application behaviour, I developed a self-contained C++20 benchmark suite (`bench_runner.cpp`) that replicates the exact data structures, SQLite queries, and FTXUI Element constructions under controlled test conditions.

### Noise Reduction and Statistical Rigour

- Every benchmark was executed across 5 independent runs to calculate arithmetic means and smooth out CPU frequency scaling noise.
- A high-resolution monotonic clock (`std::chrono::high_resolution_clock`) was used for precise timing measurements down to nanoseconds.
- Hardware-level dynamic heap allocation tracking was implemented by intercepting global `operator new` and `operator delete` to record exact heap allocation counts and byte totals in real time.

### Directory Layout

- `bench_runner.cpp`: C++20 benchmark suite executing isolated before-and-after tests.
- `run_benchmarks.sh`: Bash driver script automating compilation, execution, and raw log export.
- `raw/`: Raw, unformatted benchmark outputs for independent verification:
  - `raw/opt1_database_caching.txt`: Benchmark 1 raw output.
  - `raw/opt2_keyhint_passing.txt`: Benchmark 2 raw output.
  - `raw/opt3_monthnames_vector.txt`: Benchmark 3A raw output.
  - `raw/opt3_task_passing.txt`: Benchmark 3B raw output.
  - `raw/full_benchmark_run.txt`: Combined full suite execution log.
  - `raw/environment.txt`: Host operating system and hardware metadata.

## 3. Benchmark Results Summary

### Optimisation 1: Task List In-Memory Caching vs Repeated Database Queries

- Commit: `409736ea9aec86dca7452895e0e31cbbf52246bd`
- Test Setup: 25 tasks populated in SQLite database, simulating 1,000 render frames across 5 runs.

| Metric                    | Before (Uncached DB Query) | After (Cached In-Memory Read) | Improvement                        |
| :------------------------ | :------------------------- | :---------------------------- | :--------------------------------- |
| Average Frame Latency     | 13.43 us / frame           | 0.049 us / frame              | 99.63% reduction (272.7x speedup)  |
| Total Time (1,000 frames) | 13.43 ms                   | 0.049 ms                      | 13.38 ms saved                     |
| DB Queries per Frame      | 1 SQLite SELECT query      | 0 queries (1 load at startup) | 100% render queries eliminated     |
| Dynamic Heap Allocations  | 106 allocations / frame    | 0 allocations / frame         | 106 allocations / frame eliminated |
| Heap Churn per Frame      | 13,120 bytes / frame       | 0 bytes / frame               | 13.12 KB / frame eliminated        |

### Optimisation 2: keyHint Parameter Passing (Value vs Const Reference)

- Commit: `072e0e4258a920b59f32370d6d362bd40fa0e5ba`
- Test Setup: 8 UI keybind hints evaluated across 100,000 frames (800,000 calls total), plus 2,000,000 pure parameter calls.

| Metric                      | Before (Pass-by-Value)      | After (Pass-by-Const-Ref)   | Improvement                      |
| :-------------------------- | :-------------------------- | :-------------------------- | :------------------------------- |
| Pure Call Latency           | 8.14 ns / call              | 2.50 ns / call              | 69.35% reduction (3.26x speedup) |
| Pure Parameter Allocations  | 500,000 allocs (11.5 MB)    | 0 allocs (0 bytes)          | 100% string copies eliminated    |
| FTXUI Element Build Time    | 164.73 ms (1.65 us / frame) | 160.34 ms (1.60 us / frame) | 2.67% overall reduction          |
| FTXUI Allocations per Frame | 84 allocations / frame      | 82 allocations / frame      | 2 allocations / frame eliminated |

### Optimisation 3A: Calendar Picker monthNames (Vector vs Static Array)

- Commit: `21dbc2474ea427e65ac3dc7aec12b83799396ce0`
- Test Setup: 1,000,000 calendar header evaluations across 5 repetitions.

| Metric                           | Before (Heap std::vector)    | After (Static const char\* array) | Improvement                      |
| :------------------------------- | :--------------------------- | :-------------------------------- | :------------------------------- |
| Execution Time (1,000,000 evals) | 95.46 ms (95.46 ns / eval)   | 22.18 ms (22.18 ns / eval)        | 76.76% reduction (4.30x speedup) |
| Heap Allocations per Frame       | 1 allocation (384 bytes)     | 0 allocations (0 bytes)           | 100% heap allocation eliminated  |
| Heap Churn at 60 FPS             | 60 allocs/sec (23.04 KB/sec) | 0 allocs/sec (0 bytes/sec)        | Completely eliminated            |

### Optimisation 3B: TaskManager Const Reference Passing

- Commit: `21dbc2474ea427e65ac3dc7aec12b83799396ce0`
- Test Setup: 2,000,000 invocations with realistic Task object containing short and long strings.

| Metric                           | Before (Pass-by-Value)     | After (Pass-by-Const-Ref) | Improvement                       |
| :------------------------------- | :------------------------- | :------------------------ | :-------------------------------- |
| Execution Time (2,000,000 calls) | 45.44 ms (22.72 ns / call) | 0.57 ms (0.29 ns / call)  | 98.74% reduction (79.16x speedup) |
| Heap Allocations per Call        | 2 allocations / call       | 0 allocations / call      | 100% deep copies eliminated       |
| Dynamic Memory Allocated         | 264 MB allocated total     | 0 bytes allocated         | Zero allocation overhead          |

## 4. How to Reproduce

To compile and rerun the benchmark harness on your local machine:

```bash
chmod +x docs/performance/benchmarks/run_benchmarks.sh
./docs/performance/benchmarks/run_benchmarks.sh
```

Individual benchmarks can also be executed manually:

```bash
# Benchmark 1 only
./build/bench_runner --opt1

# Benchmark 2 only
./build/bench_runner --opt2

# Benchmark 3A only
./build/bench_runner --opt3a

# Benchmark 3B only
./build/bench_runner --opt3b
```
