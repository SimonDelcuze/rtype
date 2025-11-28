#pragma once

#include <errors/IError.hpp>
#include <string>

class NetworkSendError : public IError
{
  public:
    explicit NetworkSendError(std::string message) : message_(std::move(message)) {}

    const std::string& message() const noexcept override
    {
        return message_;
    }

  private:
    std::string message_;
};

class NetworkSendSocketError : public IError
{
  public:
    explicit NetworkSendSocketError(std::string message) : message_(std::move(message)) {}

    const std::string& message() const noexcept override
    {
        return message_;
    }

  private:
    std::string message_;
};
