cmake_minimum_required(VERSION 3.16)

project(rtype
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

include(FetchContent)

if (MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

option(BUILD_TESTS "Build tests" OFF)

file(GLOB_RECURSE RTYPE_SHARED_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/shared/src/*.cpp"
)

add_library(rtype_shared OBJECT
    ${RTYPE_SHARED_SOURCES}
)

# Fetch SFML at configure time (no external package manager needed)
set(SFML_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(SFML_BUILD_TEST_SUITE OFF CACHE INTERNAL "")
set(SFML_USE_STATIC_STD_LIBS OFF CACHE INTERNAL "")
FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(sfml)

target_include_directories(rtype_shared
    PUBLIC
        ${CMAKE_SOURCE_DIR}/shared/include
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
