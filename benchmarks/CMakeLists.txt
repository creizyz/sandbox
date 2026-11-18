
# ---- Dependencies: Google Benchmark ----

FetchContent_Declare(
        google_benchmark
        URL https://github.com/google/benchmark/archive/refs/heads/main.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# ---- Setup Google Benchmark ----

set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(google_benchmark)

# ---- Benchmark target ----

file(GLOB_RECURSE BENCH_SOURCES CONFIGURE_DEPENDS "*.cpp")
add_executable(benchmarks_run ${BENCH_SOURCES})
sandbox_enable_strict_warnings(benchmarks_run)
sandbox_add_compile_database(benchmarks_run)

target_link_libraries(benchmarks_run
        PRIVATE
        sandbox
        benchmark::benchmark_main
)
