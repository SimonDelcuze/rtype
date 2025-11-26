#pragma once

#include <errors/IError.hpp>
#include <string>

class ManifestParseError : public IError
{
  public:
    explicit ManifestParseError(std::string message) : message_(std::move(message)) {}

    const std::string& message() const noexcept override
    {
        return message_;
    }

  private:
    std::string message_;
};
