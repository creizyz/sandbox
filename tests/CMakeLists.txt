
# ---- Dependencies: Google Tests ----

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# ---- Setup Google Tests ----

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()

# ---- Tests target ----

file(GLOB_RECURSE TESTS_SOURCES CONFIGURE_DEPENDS "*.cpp")
add_executable(unit_tests ${TESTS_SOURCES})

target_link_libraries(unit_tests
        PRIVATE
        sandbox
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)