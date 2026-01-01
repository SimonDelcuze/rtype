#pragma once

#include "Common.hpp"

#include <string>

class ITexture
{
  public:
    virtual ~ITexture() = default;

    virtual bool loadFromFile(const std::string& filename)       = 0;
    virtual bool create(unsigned int width, unsigned int height) = 0;
    virtual Vector2u getSize() const                             = 0;
    virtual void setRepeated(bool repeated)                      = 0;
    virtual bool isRepeated() const                              = 0;
    virtual void setSmooth(bool smooth)                          = 0;
    virtual bool isSmooth() const                                = 0;
};
