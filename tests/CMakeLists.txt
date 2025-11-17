cmake_minimum_required(VERSION 3.16)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
set(BUILD_GMOCK OFF CACHE BOOL " " FORCE)
set(INSTALL_GTEST OFF CACHE BOOL " " FORCE)
set(gtest_disable_pthreads ON CACHE BOOL " " FORCE)

FetchContent_MakeAvailable(googletest)

add_library(rtype_gtest INTERFACE)
target_link_libraries(rtype_gtest
    INTERFACE
        GTest::gtest
        GTest::gtest_main
)

enable_testing()

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(shared)
