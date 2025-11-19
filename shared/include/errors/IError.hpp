#pragma once

#include <string>

class IError
{
  public:
    virtual ~IError()                                   = default;
    virtual const std::string& message() const noexcept = 0;
};
