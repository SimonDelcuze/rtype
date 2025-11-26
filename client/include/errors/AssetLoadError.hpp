#pragma once

#include <errors/IError.hpp>
#include <string>

/**
 * @brief Exception thrown when an asset (texture, sound, font) fails to load
 *
 * Used by TextureManager, SoundManager, and FontManager when a file cannot be
 * loaded from disk or is invalid.
 */
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
