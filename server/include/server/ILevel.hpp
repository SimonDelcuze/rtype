#pragma once

#include "server/LevelScript.hpp"

class ILevel
{
  public:
    virtual ~ILevel()                                = default;
    virtual int id() const                           = 0;
    virtual LevelScript buildScript() const          = 0;
};

