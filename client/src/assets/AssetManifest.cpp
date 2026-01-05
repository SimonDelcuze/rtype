#include "assets/AssetManifest.hpp"

#include "errors/FileNotFoundError.hpp"
#include "errors/ManifestParseError.hpp"
#include "json/Json.hpp"

#include <fstream>
#include <sstream>

using Json = rtype::Json;

AssetManifest AssetManifest::fromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw FileNotFoundError("Failed to open asset manifest: " + filepath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    try {
        Json j = Json::parse(buffer.str());
        return fromString(j.dump());
    } catch (const rtype::JsonParseError& e) {
        throw ManifestParseError("Failed to parse asset manifest: " + std::string(e.what()));
    }
}

AssetManifest AssetManifest::fromString(const std::string& jsonStr)
{
    AssetManifest manifest;

    try {
        Json j = Json::parse(jsonStr);

        if (j.contains("textures") && j["textures"].isArray()) {
            auto textures = j["textures"];
            for (size_t i=0; i < textures.size(); ++i) {
                auto item = textures[i];
                TextureEntry entry;
                entry.id   = item.getValue<std::string>("id", "");
                entry.path = item.getValue<std::string>("path", "");
                entry.type = item.getValue<std::string>("type", "");

                if (entry.id.empty() || entry.path.empty()) {
                    throw ManifestParseError("Invalid texture entry: missing id or path");
                }

                manifest.textures_.push_back(entry);
            }
        }

        if (j.contains("sounds") && j["sounds"].isArray()) {
            auto sounds = j["sounds"];
            for (size_t i=0; i < sounds.size(); ++i) {
                auto item = sounds[i];
                SoundEntry entry;
                entry.id   = item.getValue<std::string>("id", "");
                entry.path = item.getValue<std::string>("path", "");
                entry.type = item.getValue<std::string>("type", "");

                if (entry.id.empty() || entry.path.empty()) {
                    throw ManifestParseError("Invalid sound entry: missing id or path");
                }

                manifest.sounds_.push_back(entry);
            }
        }

        if (j.contains("fonts") && j["fonts"].isArray()) {
            auto fonts = j["fonts"];
            for (size_t i=0; i < fonts.size(); ++i) {
                auto item = fonts[i];
                FontEntry entry;
                entry.id   = item.getValue<std::string>("id", "");
                entry.path = item.getValue<std::string>("path", "");
                entry.type = item.getValue<std::string>("type", "");

                if (entry.id.empty() || entry.path.empty()) {
                    throw ManifestParseError("Invalid font entry: missing id or path");
                }

                manifest.fonts_.push_back(entry);
            }
        }
    } catch (const std::exception& e) {
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

std::optional<TextureEntry> AssetManifest::findTextureById(const std::string& id) const
{
    for (const auto& entry : textures_) {
        if (entry.id == id) {
            return entry;
        }
    }
    return std::nullopt;
}

std::optional<SoundEntry> AssetManifest::findSoundById(const std::string& id) const
{
    for (const auto& entry : sounds_) {
        if (entry.id == id) {
            return entry;
        }
    }
    return std::nullopt;
}
