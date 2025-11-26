#pragma once

#include <errors/IError.hpp>
#include <string>

/**
 * @brief Exception thrown when audio operations fail
 *
 * Used by audio systems when sound playback or audio device operations fail.
 */
class AudioError : public IError
{
  public:
    explicit AudioError(std::string message) : message_(std::move(message)) {}

    const std::string& message() const noexcept override
    {
        return message_;
    }

  private:
    std::string message_;
};
