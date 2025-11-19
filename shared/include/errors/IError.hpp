#pragma once

#include <exception>
#include <string>

class IError : public std::exception
{
  public:
    ~IError() override                                  = default;
    virtual const std::string& message() const noexcept = 0;

    const char* what() const noexcept override
    {
        return message().c_str();
    }
};
