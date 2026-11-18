cmake_minimum_required(VERSION 3.20)

file(READ "${DATABASE}" database)
string(JSON entry_count LENGTH "${database}")

# Filter by object directory so sources shared by multiple targets keep the correct flags.
set(object_directory "CMakeFiles/${TARGET_NAME}.dir/")
set(entries "")
set(separator "")
set(index 0)
while(index LESS entry_count)
    string(JSON entry GET "${database}" ${index})
    string(JSON object_path ERROR_VARIABLE missing_output GET "${entry}" output)
    if(missing_output)
        # Older CMake generators omit the output field but name the object in the command.
        string(JSON object_path GET "${entry}" command)
    endif()
    string(REPLACE "\\" "/" object_path "${object_path}")
    string(FIND "${object_path}" "${object_directory}" target_position)
    if(NOT target_position EQUAL -1)
        string(APPEND entries "${separator}${entry}")
        set(separator ",\n")
    endif()
    math(EXPR index "${index} + 1")
endwhile()

if(entries STREQUAL "")
    message(FATAL_ERROR "No compile commands for ${TARGET_NAME} in ${DATABASE}")
endif()

get_filename_component(target_directory "${TARGET_DATABASE}" DIRECTORY)
file(MAKE_DIRECTORY "${target_directory}")
file(WRITE "${TARGET_DATABASE}" "[\n${entries}\n]\n")
