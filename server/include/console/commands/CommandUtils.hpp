#pragma once

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace CommandUtils
{
    inline std::string toLower(const std::string& str)
    {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    inline bool startsWithIgnoreCase(const std::string& str, const std::string& prefix)
    {
        if (str.size() < prefix.size())
            return false;
        return toLower(str.substr(0, prefix.size())) == toLower(prefix);
    }

    inline std::string formatBytes(std::size_t bytes)
    {
        if (bytes < 1024)
            return std::to_string(bytes) + " B";
        if (bytes < 1024 * 1024)
            return std::to_string(bytes / 1024) + " KB";
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
        return ss.str();
    }
}
