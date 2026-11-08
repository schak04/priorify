#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <new>
#include <atomic>
#include <fstream>
#include <sstream>
#include <cstring>

#include <sqlite3.h>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

// Memory allocation tracking
static std::atomic<size_t> g_alloc_count{0};
static std::atomic<size_t> g_alloc_bytes{0};
static bool g_track_allocations = false;

void* operator new(size_t size) {
    if (g_track_allocations) {
        g_alloc_count.fetch_add(1, std::memory_order_relaxed);
        g_alloc_bytes.fetch_add(size, std::memory_order_relaxed);
    }
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void operator delete(void* p) noexcept {
    std::free(p);
}

void operator delete(void* p, size_t) noexcept {
    std::free(p);
}

struct TaskBench {
    int id;
    int priority;
    std::string taskName;
    std::string taskDesc;
    std::string date;
    bool completed;
    int position;
};

// Database helper
sqlite3* setup_benchmark_db(int task_count) {
    sqlite3* db = nullptr;
    int rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to open in-memory db: " << sqlite3_errmsg(db) << "\n";
        return nullptr;
    }

    const char* schema = R"(
        CREATE TABLE tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            description TEXT,
            due_date TEXT,
            priority INTEGER,
            completed INTEGER,
            position INTEGER NOT NULL DEFAULT 0
        );
    )";
    sqlite3_exec(db, schema, nullptr, nullptr, nullptr);

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    const char* insert_sql = "INSERT INTO tasks (name, description, due_date, priority, completed, position) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr);

    for (int i = 1; i <= task_count; i++) {
        std::string name = "Task Item " + std::to_string(i) + ": Complete system report";
        std::string desc = "Detailed description for task " + std::to_string(i) + " covering benchmarking, profiling, and documentation";
        std::string date = "15-11-2026";
        int prio = (i % 3) + 1;
        int comp = (i % 2);
        int pos = i;

        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, desc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, date.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, prio);
        sqlite3_bind_int(stmt, 5, comp);
        sqlite3_bind_int(stmt, 6, pos);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);

    return db;
}

std::vector<TaskBench> fetch_tasks_from_db(sqlite3* db) {
    std::vector<TaskBench> tasks;
    const char* sql = "SELECT id, name, description, due_date, priority, completed, position FROM tasks ORDER BY priority ASC, position ASC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return tasks;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TaskBench t;
        t.id = sqlite3_column_int(stmt, 0);
        t.taskName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t.taskDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        t.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        t.priority = sqlite3_column_int(stmt, 4);
        t.completed = sqlite3_column_int(stmt, 5);
        t.position = sqlite3_column_int(stmt, 6);
        tasks.push_back(t);
    }
    sqlite3_finalize(stmt);
    return tasks;
}

// Optimisation 1: Database Caching Benchmark
void benchmark_optimisation_1(std::ostream& out) {
    out << "=======================================================================\n";
    out << "BENCHMARK 1: TASK LIST CACHING VS UNCACHED DATABASE QUERIES\n";
    out << "Commit: 409736ea9aec86dca7452895e0e31cbbf52246bd\n";
    out << "=======================================================================\n";

    const int task_count = 25;
    sqlite3* db = setup_benchmark_db(task_count);
    if (!db) return;

    const int frames = 1000;
    const int runs = 5;

    out << "Configuration:\n";
    out << "  Task Count in Database: " << task_count << "\n";
    out << "  Simulated Render Frames per Run: " << frames << "\n";
    out << "  Benchmark Repetitions: " << runs << "\n\n";

    // Uncached runs: query DB every frame
    std::vector<double> uncached_times_ms(runs);
    size_t uncached_allocs_total = 0;
    size_t uncached_bytes_total = 0;

    for (int r = 0; r < runs; r++) {
        g_alloc_count = 0;
        g_alloc_bytes = 0;
        g_track_allocations = (r == 0);

        auto start = std::chrono::high_resolution_clock::now();
        volatile size_t check_sum = 0;
        for (int f = 0; f < frames; f++) {
            auto tasks = fetch_tasks_from_db(db);
            check_sum += tasks.size();
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        uncached_times_ms[r] = elapsed.count();

        if (r == 0) {
            g_track_allocations = false;
            uncached_allocs_total = g_alloc_count.load();
            uncached_bytes_total = g_alloc_bytes.load();
        }
    }

    // Cached runs: pre-queried cache, access in-memory every frame
    std::vector<TaskBench> cached_tasks = fetch_tasks_from_db(db);
    std::vector<double> cached_times_ms(runs);
    size_t cached_allocs_total = 0;
    size_t cached_bytes_total = 0;

    for (int r = 0; r < runs; r++) {
        g_alloc_count = 0;
        g_alloc_bytes = 0;
        g_track_allocations = (r == 0);

        auto start = std::chrono::high_resolution_clock::now();
        volatile size_t check_sum = 0;
        for (int f = 0; f < frames; f++) {
            check_sum += cached_tasks.size();
            for (const auto& t : cached_tasks) {
                check_sum += t.priority;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        cached_times_ms[r] = elapsed.count();

        if (r == 0) {
            g_track_allocations = false;
            cached_allocs_total = g_alloc_count.load();
            cached_bytes_total = g_alloc_bytes.load();
        }
    }

    auto calc_mean = [](const std::vector<double>& v) {
        return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    };

    double uncached_mean = calc_mean(uncached_times_ms);
    double cached_mean = calc_mean(cached_times_ms);

    out << std::fixed << std::setprecision(4);
    out << "--- Results: Uncached (Querying DB Every Frame) ---\n";
    out << "  Total Execution Time (mean of " << runs << " runs): " << uncached_mean << " ms\n";
    out << "  Average Frame Latency: " << (uncached_mean * 1000.0 / frames) << " us/frame\n";
    out << "  Database Queries Executed: " << frames << " queries\n";
    out << "  Dynamic Memory Allocations: " << uncached_allocs_total << " (" << (uncached_allocs_total / frames) << " per frame)\n";
    out << "  Total Heap Bytes Allocated: " << uncached_bytes_total << " bytes (" << (uncached_bytes_total / frames) << " bytes/frame)\n\n";

    out << "--- Results: Cached (In-Memory Vector Read) ---\n";
    out << "  Total Execution Time (mean of " << runs << " runs): " << cached_mean << " ms\n";
    out << "  Average Frame Latency: " << (cached_mean * 1000.0 / frames) << " us/frame\n";
    out << "  Database Queries Executed: 0 queries (1 initial load)\n";
    out << "  Dynamic Memory Allocations: " << cached_allocs_total << " (" << (cached_allocs_total / frames) << " per frame)\n";
    out << "  Total Heap Bytes Allocated: " << cached_bytes_total << " bytes (" << (cached_bytes_total / frames) << " bytes/frame)\n\n";

    double speedup = uncached_mean / cached_mean;
    out << "--- Comparison & Improvement ---\n";
    out << "  Latency Reduction: " << ((uncached_mean - cached_mean) / uncached_mean * 100.0) << " %\n";
    out << "  Throughput Speedup Factor: " << speedup << "x\n";
    out << "  Database Query Elimination: 100% of render-loop database roundtrips eliminated\n";
    out << "  Per-frame Allocation Overhead Eliminated: " << (uncached_allocs_total / frames) << " allocations/frame\n\n";

    sqlite3_close(db);
}

// Optimisation 2: keyHint Parameter Passing Benchmark
ftxui::Element keyHint_by_value(std::string key, std::string purpose) {
    return ftxui::hbox({
        ftxui::text(key) | ftxui::bold | ftxui::color(ftxui::Color::Yellow),
        ftxui::text(": " + purpose + "  ") | ftxui::dim
    });
}

ftxui::Element keyHint_by_const_ref(const std::string& key, const std::string& purpose) {
    return ftxui::hbox({
        ftxui::text(" " + key + ": ") | ftxui::bold | ftxui::color(ftxui::Color::Yellow),
        ftxui::text(purpose + "  ") | ftxui::dim
    });
}

void benchmark_optimisation_2(std::ostream& out) {
    out << "=======================================================================\n";
    out << "BENCHMARK 2: KEYHINT PASS-BY-VALUE VS PASS-BY-CONST-REFERENCE\n";
    out << "Commit: 072e0e4258a920b59f32370d6d362bd40fa0e5ba\n";
    out << "=======================================================================\n";

    const int iterations = 100000;
    const int runs = 5;

    const std::vector<std::pair<std::string, std::string>> hints = {
        {"a", "Add"},
        {"e", "Edit"},
        {"d", "Delete"},
        {"D", "Clear all"},
        {"c", "Toggle completion status"},
        {"J/K", "Move within priority"},
        {"q/Esc", "Quit"},
        {"?", "About"}
    };

    out << "Configuration:\n";
    out << "  Total Frames Simulated: " << iterations << "\n";
    out << "  Keybind Hints per Frame: " << hints.size() << " (Total calls: " << (iterations * hints.size()) << ")\n";
    out << "  Benchmark Repetitions: " << runs << "\n\n";

    out << "--- Scenario A: Pure Parameter Passing & String Copy Cost ---\n";

    auto pure_by_val = [](std::string k, std::string p) {
        volatile size_t s = k.size() + p.size();
        (void)s;
    };

    auto pure_by_ref = [](const std::string& k, const std::string& p) {
        volatile size_t s = k.size() + p.size();
        (void)s;
    };

    const int pure_iterations = 2000000;
    std::vector<double> val_times(runs);
    size_t val_allocs = 0;
    size_t val_bytes = 0;

    for (int r = 0; r < runs; r++) {
        g_alloc_count = 0;
        g_alloc_bytes = 0;
        g_track_allocations = (r == 0);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < pure_iterations; i++) {
            const auto& item = hints[i % hints.size()];
            pure_by_val(item.first, item.second);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> el = end - start;
        val_times[r] = el.count();

        if (r == 0) {
            g_track_allocations = false;
            val_allocs = g_alloc_count.load();
            val_bytes = g_alloc_bytes.load();
        }
    }

    std::vector<double> ref_times(runs);
    size_t ref_allocs = 0;
    size_t ref_bytes = 0;

    for (int r = 0; r < runs; r++) {
        g_alloc_count = 0;
        g_alloc_bytes = 0;
        g_track_allocations = (r == 0);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < pure_iterations; i++) {
            const auto& item = hints[i % hints.size()];
            pure_by_ref(item.first, item.second);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> el = end - start;
        ref_times[r] = el.count();

        if (r == 0) {
            g_track_allocations = false;
            ref_allocs = g_alloc_count.load();
            ref_bytes = g_alloc_bytes.load();
        }
    }

    auto calc_mean = [](const std::vector<double>& v) {
        return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    };

    double val_mean = calc_mean(val_times);
    double ref_mean = calc_mean(ref_times);

    out << std::fixed << std::setprecision(4);
    out << "  Pass-by-value (" << pure_iterations << " calls):\n";
    out << "    Total Time: " << val_mean << " ms (" << (val_mean * 1e6 / pure_iterations) << " ns/call)\n";
    out << "    Dynamic Heap Allocations: " << val_allocs << " (Strings >15 chars trigger heap allocation)\n";
    out << "    Dynamic Heap Allocated: " << val_bytes << " bytes\n";
    out << "  Pass-by-const-ref (" << pure_iterations << " calls):\n";
    out << "    Total Time: " << ref_mean << " ms (" << (ref_mean * 1e6 / pure_iterations) << " ns/call)\n";
    out << "    Dynamic Heap Allocations: " << ref_allocs << " (Zero heap allocations)\n";
    out << "    Dynamic Heap Allocated: " << ref_bytes << " bytes\n";
    out << "    Speedup Factor: " << (val_mean / ref_mean) << "x\n";
    out << "    Latency Reduction: " << ((val_mean - ref_mean) / val_mean * 100.0) << " %\n\n";

    out << "--- Scenario B: Full Element Construction Over 100,000 Frames ---\n";
    std::vector<double> ftxui_val_times(runs);
    size_t ftxui_val_allocs = 0;
    size_t ftxui_val_bytes = 0;

    for (int r = 0; r < runs; r++) {
        g_alloc_count = 0;
        g_alloc_bytes = 0;
        g_track_allocations = (r == 0);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; i++) {
            for (const auto& item : hints) {
                auto elem = keyHint_by_value(item.first, item.second);
                (void)elem;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> el = end - start;
        ftxui_val_times[r] = el.count();

        if (r == 0) {
            g_track_allocations = false;
            ftxui_val_allocs = g_alloc_count.load();
            ftxui_val_bytes = g_alloc_bytes.load();
        }
    }

    std::vector<double> ftxui_ref_times(runs);
    size_t ftxui_ref_allocs = 0;
    size_t ftxui_ref_bytes = 0;

    for (int r = 0; r < runs; r++) {
        g_alloc_count = 0;
        g_alloc_bytes = 0;
        g_track_allocations = (r == 0);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; i++) {
            for (const auto& item : hints) {
                auto elem = keyHint_by_const_ref(item.first, item.second);
                (void)elem;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> el = end - start;
        ftxui_ref_times[r] = el.count();

        if (r == 0) {
            g_track_allocations = false;
            ftxui_ref_allocs = g_alloc_count.load();
            ftxui_ref_bytes = g_alloc_bytes.load();
        }
    }

    double ftxui_val_mean = calc_mean(ftxui_val_times);
    double ftxui_ref_mean = calc_mean(ftxui_ref_times);

    out << "  Full Element Construction (Before: Lambda with Pass-by-Value):\n";
    out << "    Total Time (" << iterations << " frames, 8 hints/frame): " << ftxui_val_mean << " ms\n";
    out << "    Average Cost per Frame (8 hints): " << (ftxui_val_mean * 1000.0 / iterations) << " us\n";
    out << "    Total Heap Allocations: " << ftxui_val_allocs << "\n";
    out << "    Allocations per Frame: " << (ftxui_val_allocs / iterations) << "\n";
    out << "  Full Element Construction (After: Global Function with Pass-by-Const-Ref):\n";
    out << "    Total Time (" << iterations << " frames, 8 hints/frame): " << ftxui_ref_mean << " ms\n";
    out << "    Average Cost per Frame (8 hints): " << (ftxui_ref_mean * 1000.0 / iterations) << " us\n";
    out << "    Total Heap Allocations: " << ftxui_ref_allocs << "\n";
    out << "    Allocations per Frame: " << (ftxui_ref_allocs / iterations) << "\n";
    out << "  Improvement:\n";
    out << "    Time Saved: " << (ftxui_val_mean - ftxui_ref_mean) << " ms (" << ((ftxui_val_mean - ftxui_ref_mean) / ftxui_val_mean * 100.0) << " % reduction)\n";
    out << "    Allocations Saved: " << (ftxui_val_allocs - ftxui_ref_allocs) << " allocations (" << ((ftxui_val_allocs - ftxui_ref_allocs) / iterations) << " fewer per frame)\n\n";
}

// Optimisation 3A: Calendar Picker monthNames Benchmark
void benchmark_optimisation_3a(std::ostream& out) {
    out << "=======================================================================\n";
    out << "BENCHMARK 3A: STATIC ARRAY VS HEAP VECTOR (CALENDAR MONTHNAMES)\n";
    out << "Commit: 21dbc2474ea427e65ac3dc7aec12b83799396ce0\n";
    out << "=======================================================================\n";

    const int iterations = 1000000;
    const int runs = 5;

    out << "Configuration:\n";
    out << "  Evaluations: " << iterations << "\n";
    out << "  Benchmark Repetitions: " << runs << "\n\n";

    auto eval_vector_month = [](int month, int year) -> std::string {
        std::vector<std::string> monthNames = {"January", "February", "March", "April", "May",
                                               "June", "July", "August", "September", "October",
                                               "November", "December"};
        return monthNames[month - 1] + " " + std::to_string(year);
    };

    auto eval_static_month = [](int month, int year) -> std::string {
        static const char* const monthNames[] = {"January", "February", "March", "April", "May",
                                                 "June", "July", "August", "September", "October",
                                                 "November", "December"};
        return std::string(monthNames[month - 1]) + " " + std::to_string(year);
    };

    std::vector<double> vec_times(runs);
    size_t vec_allocs = 0;
    size_t vec_bytes = 0;

    for (int r = 0; r < runs; r++) {
        g_alloc_count = 0;
        g_alloc_bytes = 0;
        g_track_allocations = (r == 0);

        auto start = std::chrono::high_resolution_clock::now();
        volatile size_t len_sum = 0;
        for (int i = 0; i < iterations; i++) {
            std::string res = eval_vector_month((i % 12) + 1, 2026);
            len_sum += res.size();
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> el = end - start;
        vec_times[r] = el.count();

        if (r == 0) {
            g_track_allocations = false;
            vec_allocs = g_alloc_count.load();
            vec_bytes = g_alloc_bytes.load();
        }
    }

    std::vector<double> arr_times(runs);
    size_t arr_allocs = 0;
    size_t arr_bytes = 0;

    for (int r = 0; r < runs; r++) {
        g_alloc_count = 0;
        g_alloc_bytes = 0;
        g_track_allocations = (r == 0);

        auto start = std::chrono::high_resolution_clock::now();
        volatile size_t len_sum = 0;
        for (int i = 0; i < iterations; i++) {
            std::string res = eval_static_month((i % 12) + 1, 2026);
            len_sum += res.size();
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> el = end - start;
        arr_times[r] = el.count();

        if (r == 0) {
            g_track_allocations = false;
            arr_allocs = g_alloc_count.load();
            arr_bytes = g_alloc_bytes.load();
        }
    }

    auto calc_mean = [](const std::vector<double>& v) {
        return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    };

    double vec_mean = calc_mean(vec_times);
    double arr_mean = calc_mean(arr_times);

    out << std::fixed << std::setprecision(4);
    out << "  Original (std::vector<std::string> per frame):\n";
    out << "    Execution Time (" << iterations << " frames): " << vec_mean << " ms (" << (vec_mean * 1e6 / iterations) << " ns/frame)\n";
    out << "    Dynamic Allocations: " << vec_allocs << " (" << (double)vec_allocs / iterations << " per frame)\n";
    out << "    Dynamic Bytes Allocated: " << vec_bytes << " bytes (" << (double)vec_bytes / iterations << " bytes/frame)\n";
    out << "  Optimised (static const char* const[] in .rodata):\n";
    out << "    Execution Time (" << iterations << " frames): " << arr_mean << " ms (" << (arr_mean * 1e6 / iterations) << " ns/frame)\n";
    out << "    Dynamic Allocations: " << arr_allocs << " (" << (double)arr_allocs / iterations << " per frame)\n";
    out << "    Dynamic Bytes Allocated: " << arr_bytes << " bytes (" << (double)arr_bytes / iterations << " bytes/frame)\n";
    out << "  Measured Improvement:\n";
    out << "    Execution Time Reduction: " << ((vec_mean - arr_mean) / vec_mean * 100.0) << " %\n";
    out << "    Speedup Factor: " << (vec_mean / arr_mean) << "x\n";
    out << "    Allocations Eliminated: 1 heap allocation per frame (std::vector storage buffer of 384 bytes)\n";
    out << "    Memory Churn Eliminated at 60 FPS: 60 heap allocs/sec (23,040 bytes/sec)\n\n";
}

// Optimisation 3B: TaskManager Pass-by-Const-Ref Benchmark
void benchmark_optimisation_3b(std::ostream& out) {
    out << "=======================================================================\n";
    out << "BENCHMARK 3B: TASKMANAGER PASS-BY-VALUE VS PASS-BY-CONST-REFERENCE\n";
    out << "Commit: 21dbc2474ea427e65ac3dc7aec12b83799396ce0\n";
    out << "=======================================================================\n";

    const int runs = 5;
    const int task_iters = 2000000;

    TaskBench sample_task;
    sample_task.id = 42;
    sample_task.priority = 1;
    sample_task.taskName = "Implement memory profiling benchmark";
    sample_task.taskDesc = "Comprehensive memory allocation analysis for Priorify v2.0.0 release performance documentation";
    sample_task.date = "08-11-2026";
    sample_task.completed = false;
    sample_task.position = 1;

    auto task_val = [](TaskBench t) {
        volatile size_t sz = t.taskName.size() + t.taskDesc.size();
        (void)sz;
    };

    auto task_ref = [](const TaskBench& t) {
        volatile size_t sz = t.taskName.size() + t.taskDesc.size();
        (void)sz;
    };

    std::vector<double> t_val_times(runs);
    size_t t_val_allocs = 0;
    size_t t_val_bytes = 0;

    for (int r = 0; r < runs; r++) {
        g_alloc_count = 0;
        g_alloc_bytes = 0;
        g_track_allocations = (r == 0);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < task_iters; i++) {
            task_val(sample_task);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> el = end - start;
        t_val_times[r] = el.count();

        if (r == 0) {
            g_track_allocations = false;
            t_val_allocs = g_alloc_count.load();
            t_val_bytes = g_alloc_bytes.load();
        }
    }

    std::vector<double> t_ref_times(runs);
    size_t t_ref_allocs = 0;
    size_t t_ref_bytes = 0;

    for (int r = 0; r < runs; r++) {
        g_alloc_count = 0;
        g_alloc_bytes = 0;
        g_track_allocations = (r == 0);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < task_iters; i++) {
            task_ref(sample_task);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> el = end - start;
        t_ref_times[r] = el.count();

        if (r == 0) {
            g_track_allocations = false;
            t_ref_allocs = g_alloc_count.load();
            t_ref_bytes = g_alloc_bytes.load();
        }
    }

    auto calc_mean = [](const std::vector<double>& v) {
        return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    };

    double t_val_mean = calc_mean(t_val_times);
    double t_ref_mean = calc_mean(t_ref_times);

    out << std::fixed << std::setprecision(4);
    out << "  Task Passing (addTask / updateTask / toggleCompletionStatus):\n";
    out << "  Pass-by-value (" << task_iters << " calls):\n";
    out << "    Total Time: " << t_val_mean << " ms (" << (t_val_mean * 1e6 / task_iters) << " ns/call)\n";
    out << "    Dynamic Allocations: " << t_val_allocs << " (" << ((double)t_val_allocs / task_iters) << " per call for non-SSO strings)\n";
    out << "    Heap Bytes Allocated: " << t_val_bytes << " bytes\n";
    out << "  Pass-by-const-ref (" << task_iters << " calls):\n";
    out << "    Total Time: " << t_ref_mean << " ms (" << (t_ref_mean * 1e6 / task_iters) << " ns/call)\n";
    out << "    Dynamic Allocations: " << t_ref_allocs << " (0 allocations)\n";
    out << "    Heap Bytes Allocated: " << t_ref_bytes << " bytes\n";
    out << "  Improvement:\n";
    out << "    Speedup Factor: " << (t_val_mean / t_ref_mean) << "x\n";
    out << "    Latency Reduction: " << ((t_val_mean - t_ref_mean) / t_val_mean * 100.0) << " %\n";
    out << "    Dynamic Allocations Completely Avoided on Every Task Handler Call\n\n";
}

int main(int argc, char** argv) {
    bool run_all = true;
    bool run_1 = false;
    bool run_2 = false;
    bool run_3a = false;
    bool run_3b = false;

    if (argc > 1) {
        run_all = false;
        for (int i = 1; i < argc; i++) {
            if (std::strcmp(argv[i], "--all") == 0) run_all = true;
            else if (std::strcmp(argv[i], "--opt1") == 0) run_1 = true;
            else if (std::strcmp(argv[i], "--opt2") == 0) run_2 = true;
            else if (std::strcmp(argv[i], "--opt3a") == 0) run_3a = true;
            else if (std::strcmp(argv[i], "--opt3b") == 0) run_3b = true;
        }
    }

    if (run_all) {
        benchmark_optimisation_1(std::cout);
        benchmark_optimisation_2(std::cout);
        benchmark_optimisation_3a(std::cout);
        benchmark_optimisation_3b(std::cout);
    } else {
        if (run_1) benchmark_optimisation_1(std::cout);
        if (run_2) benchmark_optimisation_2(std::cout);
        if (run_3a) benchmark_optimisation_3a(std::cout);
        if (run_3b) benchmark_optimisation_3b(std::cout);
    }

    return 0;
}
