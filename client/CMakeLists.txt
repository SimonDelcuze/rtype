file(GLOB_RECURSE RTYPE_CLIENT_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

set(SFML_LIBS SFML::Graphics SFML::Window SFML::System SFML::Audio)

list(FILTER RTYPE_CLIENT_SOURCES EXCLUDE REGEX "Main\\.cpp$")

add_library(rtype_client_lib STATIC
    ${RTYPE_CLIENT_SOURCES}
)

target_compile_options(rtype_client_lib PRIVATE ${RTYPE_COMPILE_OPTIONS})

target_include_directories(rtype_client_lib
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rtype_client_lib
    PUBLIC
        rtype_shared
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
        SFML::Network
        nlohmann_json::nlohmann_json
        $<$<BOOL:${WIN32}>:ws2_32>
)

add_executable(rtype_client
    src/Main.cpp
)

target_link_libraries(rtype_client
    PRIVATE
        rtype_client_lib
        ${SFML_LIBS}
        $<$<BOOL:${WIN32}>:ws2_32>
)

target_include_directories(rtype_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(rtype_client PRIVATE ${RTYPE_COMPILE_OPTIONS})
