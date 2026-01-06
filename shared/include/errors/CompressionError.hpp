#pragma once

#include "errors/IError.hpp"

#include <string>

class CompressionError : public IError
{
  public:
    explicit CompressionError(std::string message) : message_(std::move(message)) {}
    const std::string& message() const noexcept override
    {
        return message_;
    }

  private:
    std::string message_;
};

class DecompressionError : public IError
{
  public:
    explicit DecompressionError(std::string message) : message_(std::move(message)) {}
    const std::string& message() const noexcept override
    {
        return message_;
    }

  private:
    std::string message_;
};
