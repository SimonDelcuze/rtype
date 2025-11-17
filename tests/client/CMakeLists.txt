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
)

add_test(NAME client_tests COMMAND rtype_client_tests)
