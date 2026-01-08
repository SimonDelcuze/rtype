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
option(BUILD_CLIENT "Build client" ON)

file(GLOB_RECURSE RTYPE_SHARED_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/shared/src/*.cpp"
)

add_library(rtype_shared OBJECT
    ${RTYPE_SHARED_SOURCES}
)

target_compile_options(rtype_shared PRIVATE ${RTYPE_COMPILE_OPTIONS})

CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG v1.90.4
    GIT_SHALLOW TRUE
)

if(imgui_ADDED)
    find_package(OpenGL REQUIRED)
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(imgui PUBLIC OpenGL::GL)
endif()

set(LZ4_MIN_CMAKE_PATCH "${CMAKE_CURRENT_LIST_DIR}/cmake/patches/lz4_force_min_cmake.cmake")

CPMAddPackage(
    NAME lz4
    GITHUB_REPOSITORY lz4/lz4
    GIT_TAG v1.9.4
    GIT_SHALLOW TRUE
    DOWNLOAD_ONLY YES
)

if(lz4_ADDED)
    set(LZ4_CMAKELISTS "${lz4_SOURCE_DIR}/build/cmake/CMakeLists.txt")
    if(EXISTS "${LZ4_CMAKELISTS}")
        execute_process(
            COMMAND ${CMAKE_COMMAND}
                -DLZ4_CMAKELISTS=${LZ4_CMAKELISTS}
                -P ${LZ4_MIN_CMAKE_PATCH}
            WORKING_DIRECTORY ${lz4_SOURCE_DIR}
        )
    else()
        message(FATAL_ERROR "Could not find ${LZ4_CMAKELISTS}")
    endif()

    if(NOT DEFINED lz4_BINARY_DIR)
        set(lz4_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/lz4-build)
    endif()

    set(LZ4_BUILD_CLI OFF CACHE BOOL "" FORCE)
    set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "" FORCE)

    add_subdirectory(${lz4_SOURCE_DIR}/build/cmake ${lz4_BINARY_DIR})
endif()

target_include_directories(rtype_shared
    PUBLIC
        ${CMAKE_SOURCE_DIR}/shared/include
)

target_link_libraries(rtype_shared
    PUBLIC
        nlohmann_json::nlohmann_json
        lz4_static
)

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()
add_subdirectory(server)
add_subdirectory(editor)

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

if (BUILD_TESTS)
    add_custom_target(tests
        COMMAND ${CMAKE_SOURCE_DIR}/rtype_server_tests
        COMMAND ${CMAKE_SOURCE_DIR}/rtype_client_tests
        DEPENDS rtype_server_tests rtype_client_tests
        COMMENT "Running all tests"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()
