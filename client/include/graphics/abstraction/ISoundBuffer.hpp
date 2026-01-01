#pragma once

#include <string>

class ISoundBuffer
{
  public:
    virtual ~ISoundBuffer()                                = default;
    virtual bool loadFromFile(const std::string& filename) = 0;
};
