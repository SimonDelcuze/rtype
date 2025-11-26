#pragma once

#include <errors/IError.hpp>
#include <string>

/**
 * @brief Exception thrown when rendering operations fail
 *
 * Used by rendering systems when window creation, rendering, or graphics
 * operations fail.
 */
class RenderError : public IError
{
  public:
    explicit RenderError(std::string message) : message_(std::move(message)) {}

    const std::string& message() const noexcept override
    {
        return message_;
    }

  private:
    std::string message_;
};
