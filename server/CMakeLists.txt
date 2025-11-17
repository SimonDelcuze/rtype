file(GLOB_RECURSE RTYPE_SERVER_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER RTYPE_SERVER_SOURCES EXCLUDE REGEX "Main\\.cpp$")

add_library(rtype_server_lib STATIC
    ${RTYPE_SERVER_SOURCES}
)

target_include_directories(rtype_server_lib
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rtype_server_lib
    PRIVATE
        rtype_shared
)

add_executable(rtype_server
    src/Main.cpp
)

target_link_libraries(rtype_server
    PRIVATE
        rtype_server_lib
)

target_include_directories(rtype_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
