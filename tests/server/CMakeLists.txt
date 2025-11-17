file(GLOB_RECURSE RTYPE_SERVER_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(rtype_server_tests
    ${RTYPE_SERVER_TEST_SOURCES}
)

target_link_libraries(rtype_server_tests
    PRIVATE
        rtype_shared
        rtype_server_lib
        rtype_gtest
)

target_include_directories(rtype_server_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/server/include
)

add_test(NAME server_tests COMMAND rtype_server_tests)
