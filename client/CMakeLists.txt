file(GLOB_RECURSE RTYPE_CLIENT_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER RTYPE_CLIENT_SOURCES EXCLUDE REGEX "Main\\.cpp$")

add_library(rtype_client_lib STATIC
    ${RTYPE_CLIENT_SOURCES}
)

target_include_directories(rtype_client_lib
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rtype_client_lib
    PRIVATE
        rtype_shared
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
        SFML::Network
)

add_executable(rtype_client
    src/Main.cpp
)

target_link_libraries(rtype_client
    PRIVATE
        rtype_client_lib
)

target_include_directories(rtype_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
