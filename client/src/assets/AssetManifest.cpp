#include "assets/AssetManifest.hpp"

#include "errors/FileNotFoundError.hpp"
#include "errors/ManifestParseError.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

AssetManifest AssetManifest::fromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw FileNotFoundError("Failed to open asset manifest: " + filepath);
    }

    json j;
    try {
        file >> j;
    } catch (const json::parse_error& e) {
        throw ManifestParseError("Failed to parse asset manifest: " + std::string(e.what()));
    }

    return fromString(j.dump());
}

AssetManifest AssetManifest::fromString(const std::string& jsonStr)
{
    AssetManifest manifest;

    try {
        json j = json::parse(jsonStr);

        if (j.contains("textures") && j["textures"].is_array()) {
            for (const auto& item : j["textures"]) {
                TextureEntry entry;
                entry.id   = item.value("id", "");
                entry.path = item.value("path", "");
                entry.type = item.value("type", "");

                if (entry.id.empty() || entry.path.empty()) {
                    throw ManifestParseError("Invalid texture entry: missing id or path");
                }

                manifest.textures_.push_back(entry);
            }
        }

        if (j.contains("sounds") && j["sounds"].is_array()) {
            for (const auto& item : j["sounds"]) {
                SoundEntry entry;
                entry.id   = item.value("id", "");
                entry.path = item.value("path", "");
                entry.type = item.value("type", "");

                if (entry.id.empty() || entry.path.empty()) {
                    throw ManifestParseError("Invalid sound entry: missing id or path");
                }

                manifest.sounds_.push_back(entry);
            }
        }

        if (j.contains("fonts") && j["fonts"].is_array()) {
            for (const auto& item : j["fonts"]) {
                FontEntry entry;
                entry.id   = item.value("id", "");
                entry.path = item.value("path", "");
                entry.type = item.value("type", "");

                if (entry.id.empty() || entry.path.empty()) {
                    throw ManifestParseError("Invalid font entry: missing id or path");
                }

                manifest.fonts_.push_back(entry);
            }
        }
    } catch (const json::exception& e) {
        throw ManifestParseError("Failed to parse asset manifest JSON: " + std::string(e.what()));
    }

    return manifest;
}

const std::vector<TextureEntry>& AssetManifest::getTextures() const
{
    return textures_;
}

std::vector<TextureEntry> AssetManifest::getTexturesByType(const std::string& type) const
{
    std::vector<TextureEntry> result;
    for (const auto& entry : textures_) {
        if (entry.type == type) {
            result.push_back(entry);
        }
    }
    return result;
}

const std::vector<SoundEntry>& AssetManifest::getSounds() const
{
    return sounds_;
}

std::vector<SoundEntry> AssetManifest::getSoundsByType(const std::string& type) const
{
    std::vector<SoundEntry> result;
    for (const auto& entry : sounds_) {
        if (entry.type == type) {
            result.push_back(entry);
        }
    }
    return result;
}

const std::vector<FontEntry>& AssetManifest::getFonts() const
{
    return fonts_;
}

std::vector<FontEntry> AssetManifest::getFontsByType(const std::string& type) const
{
    std::vector<FontEntry> result;
    for (const auto& entry : fonts_) {
        if (entry.type == type) {
            result.push_back(entry);
        }
    }
    return result;
}
