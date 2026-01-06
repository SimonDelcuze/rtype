if(NOT DEFINED LZ4_CMAKELISTS)
    message(FATAL_ERROR "LZ4_CMAKELISTS is not set")
endif()

file(READ "${LZ4_CMAKELISTS}" _lz4_content)
set(_pattern "cmake_minimum_required\\(VERSION [^\\)]+\\)")
string(REGEX MATCH "${_pattern}" _existing "${_lz4_content}")

if(NOT _existing)
    message(FATAL_ERROR "Could not find cmake_minimum_required in ${LZ4_CMAKELISTS}")
endif()

string(REGEX REPLACE "${_pattern}" "cmake_minimum_required(VERSION 3.5)" _lz4_patched "${_lz4_content}")

if(_lz4_patched STREQUAL _lz4_content)
    message(STATUS "No cmake_minimum_required change needed for ${LZ4_CMAKELISTS}")
else()
    file(WRITE "${LZ4_CMAKELISTS}" "${_lz4_patched}")
    message(STATUS "Patched ${LZ4_CMAKELISTS} to require CMake 3.5")
endif()
