cmake_minimum_required(VERSION 3.16)

project(rtype
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

if (MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(BUILD_TESTS "Build tests" OFF)

file(GLOB_RECURSE RTYPE_SHARED_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/shared/src/*.c"
    "${CMAKE_SOURCE_DIR}/shared/src/*.cpp"
)

add_library(rtype_shared OBJECT
    ${RTYPE_SHARED_SOURCES}
)

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
