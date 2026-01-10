    file(GLOB_RECURSE RTYPE_SERVER_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER RTYPE_SERVER_SOURCES EXCLUDE REGEX "Main\\.cpp$")

add_library(rtype_server_lib STATIC
    ${RTYPE_SERVER_SOURCES}
)

target_compile_options(rtype_server_lib PRIVATE ${RTYPE_COMPILE_OPTIONS})

target_include_directories(rtype_server_lib
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/shared/include
)

target_link_libraries(rtype_server_lib
    PRIVATE
        rtype_shared
        nlohmann_json::nlohmann_json
        sqlite3
        jwt-cpp::jwt-cpp
        OpenSSL::SSL
        OpenSSL::Crypto
)

add_executable(rtype_server
    src/core/Main.cpp
)

target_link_libraries(rtype_server
    PRIVATE
        rtype_server_lib
        $<$<BOOL:${WIN32}>:ws2_32>
)

target_include_directories(rtype_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/shared/include
)

target_compile_options(rtype_server PRIVATE ${RTYPE_COMPILE_OPTIONS})

set_target_properties(rtype_server PROPERTIES 
    OUTPUT_NAME "r-type_server"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
