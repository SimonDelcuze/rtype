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
        $<$<BOOL:${WIN32}>:ws2_32>
)

target_include_directories(rtype_server_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/server/include
        ${CMAKE_SOURCE_DIR}/shared/include
)

add_test(NAME server_tests COMMAND rtype_server_tests)

set_target_properties(rtype_server_tests PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_tests_properties(server_tests PROPERTIES
    FIXTURES_SETUP server_assets
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_command(
    TARGET rtype_server_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/server
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/server/assets
        ${CMAKE_BINARY_DIR}/server/assets
    COMMENT "Copying server assets for tests"
)
