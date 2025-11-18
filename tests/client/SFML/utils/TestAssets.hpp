#pragma once

#include <filesystem>
#include <string>

#ifndef RTYPE_ASSETS_DIR
#define RTYPE_ASSETS_DIR "client/assets"
#endif

inline std::string assetPath(const std::string& relative)
{
    return (std::filesystem::path(RTYPE_ASSETS_DIR) / relative).string();
}
