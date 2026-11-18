if(CMAKE_EXPORT_COMPILE_COMMANDS AND CMAKE_GENERATOR MATCHES "Ninja|Makefiles|WMake")
    add_custom_target(compile_databases)
endif()

function(sandbox_add_compile_database target)
    if(NOT TARGET compile_databases)
        return()
    endif()

    set(database "${CMAKE_BINARY_DIR}/compile_commands.json")
    set(target_database "${CMAKE_BINARY_DIR}/compile_commands/${target}/compile_commands.json")
    set(script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/WriteTargetCompileDatabase.cmake")

    add_custom_command(
        OUTPUT "${target_database}"
        COMMAND "${CMAKE_COMMAND}"
            "-DDATABASE=${database}"
            "-DTARGET_DATABASE=${target_database}"
            "-DTARGET_NAME=${target}"
            -P "${script}"
        DEPENDS "${database}" "${script}"
        VERBATIM
    )
    add_custom_target(${target}_compile_database DEPENDS "${target_database}")
    add_dependencies(${target} ${target}_compile_database)
    add_dependencies(compile_databases ${target}_compile_database)
endfunction()
