#pragma once

#include <string>

class IFont {
public:
    virtual ~IFont() = default;
    
    virtual bool loadFromFile(const std::string& filename) = 0;
};
