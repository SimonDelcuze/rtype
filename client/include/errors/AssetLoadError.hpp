#pragma once

#include <errors/IError.hpp>
#include <string>

class AssetLoadError : public IError
{
  public:
    explicit AssetLoadError(std::string message) : message_(std::move(message)) {}

    const std::string& message() const noexcept override
    {
        return message_;
    }

  private:
    std::string message_;
};
