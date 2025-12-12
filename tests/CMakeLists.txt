cmake_minimum_required(VERSION 3.16)

include(${CPM_DOWNLOAD_LOCATION})
CPMAddPackage(
    NAME GTest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG v1.14.0
    OPTIONS
        "INSTALL_GTEST OFF"
        "gtest_disable_pthreads ON"
        "BUILD_GMOCK OFF"
)

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
