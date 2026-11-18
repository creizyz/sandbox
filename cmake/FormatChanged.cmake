option(SANDBOX_FORMAT_BEFORE_BUILD "Auto-format changed Sandbox C++ files before compilation" ON)

if(SANDBOX_FORMAT_BEFORE_BUILD AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    find_package(Git REQUIRED)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    find_program(SANDBOX_CLANG_FORMAT NAMES clang-format-18 clang-format REQUIRED)

    add_custom_target(format_changed
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/format_changed.py" apply
            --git "${GIT_EXECUTABLE}" --clang-format "${SANDBOX_CLANG_FORMAT}"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Formatting changed Sandbox C++ files"
        VERBATIM
    )
    foreach(target IN ITEMS sandbox unit_tests benchmarks_run)
        if(TARGET ${target})
            add_dependencies(${target} format_changed)
        endif()
    endforeach()
endif()
