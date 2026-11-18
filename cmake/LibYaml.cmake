function(sandbox_add_libyaml)
    # LibYAML 0.2.5 declares a CMake policy version older than CMake 4 supports.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(BUILD_TESTING OFF)

    FetchContent_Declare(
        libyaml
        URL https://codeload.github.com/yaml/libyaml/tar.gz/refs/tags/0.2.5
        URL_HASH SHA256=fa240dbf262be053f3898006d502d514936c818e422afdcf33921c63bed9bf2e
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(libyaml)
endfunction()

sandbox_add_libyaml()
