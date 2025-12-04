#include "animation/AnimationManifest.hpp"

#include <fstream>
#include <iostream>

static AnimationClip parseClip(const nlohmann::json& clipJson)
{
    AnimationClip clip;
    clip.id        = clipJson.value("id", "");
    clip.frameTime = clipJson.value("frameTime", 0.1F);
    clip.loop      = clipJson.value("loop", true);
    if (clipJson.contains("frames") && clipJson["frames"].is_array()) {
        for (const auto& f : clipJson["frames"]) {
            AnimationFrame frame{};
            frame.x      = f.value("x", 0);
            frame.y      = f.value("y", 0);
            frame.width  = f.value("width", 0);
            frame.height = f.value("height", 0);
            clip.frames.push_back(frame);
        }
    }
    // optional "rows" helper for uneven sprite sheets
    if (clip.frames.empty() && clipJson.contains("rows") && clipJson["rows"].is_array()) {
        int y = 0;
        for (const auto& row : clipJson["rows"]) {
            if (!row.is_object() || !row.contains("height") || !row.contains("widths"))
                continue;
            int rowHeight = row.value("height", 0);
            int x         = 0;
            for (const auto& w : row["widths"]) {
                int cw = w.get<int>();
                clip.frames.push_back(AnimationFrame{x, y, cw, rowHeight});
                x += cw;
            }
            y += rowHeight;
        }
    }
    return clip;
}

AnimationAtlas AnimationManifest::loadFromJson(const nlohmann::json& j)
{
    AnimationAtlas atlas;
    if (!j.is_object()) {
        return atlas;
    }

    if (j.contains("animations")) {
        for (const auto& entry : j["animations"]) {
            if (!entry.is_object()) {
                continue;
            }
            AnimationClip clip = parseClip(entry);
            if (!clip.id.empty() && !clip.frames.empty()) {
                atlas.clips.registerClip(std::move(clip));
            }
        }
    }

    if (j.contains("labels") && j["labels"].is_object()) {
        for (auto it = j["labels"].begin(); it != j["labels"].end(); ++it) {
            if (!it.value().is_object()) {
                continue;
            }
            std::unordered_map<std::string, std::string> mapping;
            for (auto lit = it.value().begin(); lit != it.value().end(); ++lit) {
                if (lit.value().is_string()) {
                    mapping[lit.key()] = lit.value().get<std::string>();
                }
            }
            atlas.labels[it.key()] = std::move(mapping);
        }
    }

    return atlas;
}

AnimationAtlas AnimationManifest::loadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[AnimationManifest] Failed to open " << path << '\n';
        return AnimationAtlas{};
    }
    nlohmann::json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        std::cerr << "[AnimationManifest] Failed to parse " << path << ": " << e.what() << '\n';
        return AnimationAtlas{};
    }
    return loadFromJson(j);
}
