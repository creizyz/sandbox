cmake_minimum_required(VERSION 3.20)
project(Sandbox VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ---- Collect Sources ----
file(GLOB_RECURSE LIB_SOURCES
        CONFIGURE_DEPENDS
        "sources/*.cpp"
        "sources/*.hpp"
)

# ---- Library Target ----
add_library(sandbox ${LIB_SOURCES})
target_include_directories(sandbox PUBLIC sources)

# ---- Modules ----
option(BUILD_TESTING "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()