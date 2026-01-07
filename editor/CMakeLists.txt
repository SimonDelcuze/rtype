file(GLOB_RECURSE RTYPE_EDITOR_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(rtype_level_editor
    ${RTYPE_EDITOR_SOURCES}
)

target_include_directories(rtype_level_editor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/server/include
        ${CMAKE_SOURCE_DIR}/shared/include
)

target_link_libraries(rtype_level_editor
    PRIVATE
        imgui
        rtype_shared
        rtype_server_lib
        nlohmann_json::nlohmann_json
        SFML::Graphics
        SFML::Window
        SFML::System
        $<$<BOOL:${WIN32}>:ws2_32>
)

target_compile_options(rtype_level_editor PRIVATE ${RTYPE_COMPILE_OPTIONS})

set_target_properties(rtype_level_editor PROPERTIES OUTPUT_NAME "r-type_level_editor")
