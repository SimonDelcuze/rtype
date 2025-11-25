#include "assets/AssetManifest.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

AssetManifest AssetManifest::fromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open asset manifest: " + filepath);
    }

    json j;
    try {
        file >> j;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse asset manifest: " + std::string(e.what()));
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
                    throw std::runtime_error("Invalid texture entry: missing id or path");
                }

                manifest.textures_.push_back(entry);
            }
        }
    } catch (const json::exception& e) {
        throw std::runtime_error("Failed to parse asset manifest JSON: " + std::string(e.what()));
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
