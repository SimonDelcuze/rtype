#pragma once

#include "levels/LevelScript.hpp"

class ILevel
{
  public:
    virtual ~ILevel()                       = default;
    virtual int id() const                  = 0;
    virtual LevelScript buildScript() const = 0;
};
