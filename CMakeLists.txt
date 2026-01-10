cmake_minimum_required(VERSION 3.16)

project(rtype
    VERSION 0.1.0
    LANGUAGES CXX C
)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
    set(CMAKE_POLICY_DEFAULT_CMP0169 OLD)
endif()

if(APPLE)
    if((NOT DEFINED CMAKE_OSX_ARCHITECTURES) OR (CMAKE_OSX_ARCHITECTURES STREQUAL ""))
        if(EXISTS "/opt/homebrew/opt/openssl@3" AND NOT EXISTS "/usr/local/opt/openssl@3")
            set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
        elseif(EXISTS "/usr/local/opt/openssl@3" AND NOT EXISTS "/opt/homebrew/opt/openssl@3")
            set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "" FORCE)
        else()
            set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "" FORCE)
        endif()
    elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64" AND EXISTS "/opt/homebrew/opt/openssl@3" AND
           NOT EXISTS "/usr/local/opt/openssl@3")
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
    elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64" AND EXISTS "/usr/local/opt/openssl@3" AND
           NOT EXISTS "/opt/homebrew/opt/openssl@3")
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "" FORCE)
    endif()
    if((NOT DEFINED OPENSSL_ROOT_DIR) OR (OPENSSL_ROOT_DIR STREQUAL ""))
        if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64" AND EXISTS "/opt/homebrew/opt/openssl@3")
            set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl@3" CACHE PATH "" FORCE)
        elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64" AND EXISTS "/usr/local/opt/openssl@3")
            set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl@3" CACHE PATH "" FORCE)
        endif()
    endif()
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

set(SFML_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(SFML_BUILD_TEST_SUITE OFF CACHE INTERNAL "")
set(SFML_USE_STATIC_STD_LIBS OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(BUILD_CLIENT OR BUILD_EDITOR)
    CPMAddPackage(
        NAME SFML
        GITHUB_REPOSITORY SFML/SFML
        GIT_TAG 3.0.2
        GIT_SHALLOW TRUE
    )
endif()

CPMAddPackage(
    NAME SQLite3
    GITHUB_REPOSITORY alex85k/sqlite3-cmake
    GIT_TAG master
    GIT_SHALLOW TRUE
    DOWNLOAD_ONLY YES
)

if(SQLite3_ADDED)
    add_library(sqlite3 STATIC ${SQLite3_SOURCE_DIR}/src/sqlite3.c)
    target_include_directories(sqlite3 PUBLIC ${SQLite3_SOURCE_DIR}/src)
    target_compile_definitions(sqlite3 PUBLIC
        SQLITE_ENABLE_RTREE
        SQLITE_ENABLE_FTS4
        SQLITE_ENABLE_FTS5
        SQLITE_ENABLE_JSON1
        SQLITE_ENABLE_RBU
        SQLITE_ENABLE_STAT4
    )
    if(UNIX)
        find_package(Threads REQUIRED)
        target_link_libraries(sqlite3 PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
    endif()
endif()

CPMAddPackage(
    NAME jwt-cpp
    GITHUB_REPOSITORY Thalhammer/jwt-cpp
    GIT_TAG v0.7.0
    GIT_SHALLOW TRUE
)

find_package(OpenSSL REQUIRED)

set(LZ4_MIN_CMAKE_PATCH "${CMAKE_CURRENT_LIST_DIR}/cmake/patches/lz4_force_min_cmake.cmake")

CPMAddPackage(
    NAME lz4
    GITHUB_REPOSITORY lz4/lz4
    GIT_TAG v1.10.0
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR build/cmake
    OPTIONS
        "LZ4_BUILD_CLI OFF"
        "LZ4_BUILD_LEGACY_LZ4C OFF"
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

option(BUILD_EDITOR "Build editor" OFF)

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()
add_subdirectory(server)

if(BUILD_EDITOR)
    add_subdirectory(editor)
endif()

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
