file(GLOB_RECURSE RTYPE_SERVER_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(rtype_server
    ${RTYPE_SERVER_SOURCES}
)

target_include_directories(rtype_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rtype_server
    PRIVATE
        rtype_shared
)
