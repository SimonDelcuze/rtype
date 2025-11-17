file(GLOB_RECURSE RTYPE_CLIENT_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(rtype_client_tests
    ${RTYPE_CLIENT_TEST_SOURCES}
)

target_link_libraries(rtype_client_tests
    PRIVATE
        rtype_shared
        rtype_client_lib
        rtype_gtest
)

target_include_directories(rtype_client_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/client/include
)

add_test(NAME client_tests COMMAND rtype_client_tests)
