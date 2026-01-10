file(GLOB_RECURSE RTYPE_SHARED_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
)

add_executable(rtype_shared_tests
    ${RTYPE_SHARED_TEST_SOURCES}
)

target_link_libraries(rtype_shared_tests
    PRIVATE
        rtype_shared
        rtype_gtest
        $<$<BOOL:${WIN32}>:ws2_32>
)

add_test(NAME shared_tests COMMAND rtype_shared_tests)

set_target_properties(rtype_shared_tests PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
