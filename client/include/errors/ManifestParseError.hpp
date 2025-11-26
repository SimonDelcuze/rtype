#pragma once

#include <errors/IError.hpp>
#include <string>

/**
 * @brief Exception thrown when an asset manifest file cannot be parsed
 *
 * Used by AssetManifest when JSON parsing fails or required fields are missing.
 */
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
