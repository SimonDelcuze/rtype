cmake_minimum_required(VERSION 3.16)

project(rtype
    VERSION 0.1.0
    LANGUAGES CXX
)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
    set(CMAKE_POLICY_DEFAULT_CMP0169 OLD)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

set(CPM_DOWNLOAD_VERSION 0.38.6)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/cmake")
if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
    message(STATUS "Downloading CPM.cmake v${CPM_DOWNLOAD_VERSION}")
    file(DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${CPM_DOWNLOAD_LOCATION}"
        TLS_VERIFY ON
    )
endif()
include(${CPM_DOWNLOAD_LOCATION})

if (MSVC)
    set(RTYPE_COMPILE_OPTIONS /W4 /permissive- /EHsc)
else()
    set(RTYPE_COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Werror)
endif()

option(BUILD_TESTS "Build tests" OFF)

file(GLOB_RECURSE RTYPE_SHARED_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/shared/src/*.cpp"
)

add_library(rtype_shared OBJECT
    ${RTYPE_SHARED_SOURCES}
)

target_compile_options(rtype_shared PRIVATE ${RTYPE_COMPILE_OPTIONS})

set(SFML_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(SFML_BUILD_TEST_SUITE OFF CACHE INTERNAL "")
set(SFML_USE_STATIC_STD_LIBS OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
CPMAddPackage(
    NAME SFML
    GITHUB_REPOSITORY SFML/SFML
    GIT_TAG 3.0.2
    GIT_SHALLOW TRUE
)

CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

CPMAddPackage(
    NAME lz4
    GITHUB_REPOSITORY lz4/lz4
    GIT_TAG v1.9.4
    SOURCE_SUBDIR build/cmake
    OPTIONS "LZ4_BUILD_CLI OFF" "LZ4_BUILD_LEGACY_LZ4C OFF"
)

target_include_directories(rtype_shared
    PUBLIC
        ${CMAKE_SOURCE_DIR}/shared/include
)

target_link_libraries(rtype_shared
    PUBLIC
        nlohmann_json::nlohmann_json
        lz4_static
)

add_subdirectory(client)
add_subdirectory(server)

if (BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

add_custom_target(clang_format
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/format.sh
    COMMENT "Running clang-format on project sources"
)

add_custom_target(clang_tidy
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/lint.sh
    COMMENT "Running clang-tidy on project sources"
)
