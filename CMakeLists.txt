cmake_minimum_required(VERSION 3.20)
project(Sandbox VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED CMAKE_EXPORT_COMPILE_COMMANDS OR CMAKE_EXPORT_COMPILE_COMMANDS STREQUAL "")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

include(FetchContent)
include(cmake/CompileDatabases.cmake)

function(sandbox_enable_strict_warnings target)
    if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE /W4 /WX)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

# ---- Collect Sources ----
file(GLOB_RECURSE LIB_SOURCES
        CONFIGURE_DEPENDS
        "library/*.cpp"
        "library/*.hpp"
)

# ---- Library Target ----
add_library(sandbox ${LIB_SOURCES})
sandbox_enable_strict_warnings(sandbox)
sandbox_add_compile_database(sandbox)
target_compile_features(sandbox PUBLIC cxx_std_23)
target_include_directories(sandbox PUBLIC library)

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
