#pragma once

#include <errors/IError.hpp>
#include <string>

/**
 * @brief Exception thrown when a file cannot be found or opened
 *
 * Used when attempting to open manifest files or other configuration files.
 */
class FileNotFoundError : public IError
{
  public:
    explicit FileNotFoundError(std::string message) : message_(std::move(message)) {}

    const std::string& message() const noexcept override
    {
        return message_;
    }

  private:
    std::string message_;
};
