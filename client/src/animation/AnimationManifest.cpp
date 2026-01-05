#include "animation/AnimationManifest.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

using namespace rtype;

static AnimationClip parseClip(const Json& clipJson)
{
    AnimationClip clip;
    clip.id        = clipJson.getValue<std::string>("id", "");
    clip.frameTime = clipJson.getValue<float>("frameTime", 0.1F);
    clip.loop      = clipJson.getValue<bool>("loop", true);

    if (clipJson.contains("frames") && clipJson["frames"].isArray()) {
        auto frames = clipJson["frames"];
        for (size_t i = 0; i < frames.size(); ++i) {
            auto f = frames[i];
            AnimationFrame frame{};
            frame.x      = f.getValue<int>("x", 0);
            frame.y      = f.getValue<int>("y", 0);
            frame.width  = f.getValue<int>("width", 0);
            frame.height = f.getValue<int>("height", 0);
            clip.frames.push_back(frame);
        }
    }
    if (clip.frames.empty() && clipJson.contains("rows") && clipJson["rows"].isArray()) {
        int y     = 0;
        auto rows = clipJson["rows"];
        for (size_t i = 0; i < rows.size(); ++i) {
            auto row = rows[i];
            if (!row.isObject() || !row.contains("height") || !row.contains("widths"))
                continue;
            int rowHeight = row.getValue<int>("height", 0);
            int x         = 0;
            auto widths   = row["widths"];
            if (widths.isArray()) {
                for (size_t j = 0; j < widths.size(); ++j) {
                    int cw = widths[j].get<int>();
                    clip.frames.push_back(AnimationFrame{x, y, cw, rowHeight});
                    x += cw;
                }
            }
            y += rowHeight;
        }
    }
    return clip;
}

AnimationAtlas AnimationManifest::loadFromJson(const Json& j)
{
    AnimationAtlas atlas;
    if (!j.isObject()) {
        return atlas;
    }

    if (j.contains("animations") && j["animations"].isArray()) {
        auto animations = j["animations"];
        for (size_t i = 0; i < animations.size(); ++i) {
            auto entry = animations[i];
            if (!entry.isObject()) {
                continue;
            }
            AnimationClip clip = parseClip(entry);
            if (!clip.id.empty() && !clip.frames.empty()) {
                atlas.clips.registerClip(std::move(clip));
            }
        }
    }

    if (j.contains("labels") && j["labels"].isObject()) {
        auto labels = j["labels"];
        auto keys   = labels.getKeys();
        for (const auto& key : keys) {
            auto val = labels[key];
            if (!val.isObject()) {
                continue;
            }
            std::unordered_map<std::string, std::string> mapping;
            auto subKeys = val.getKeys();
            for (const auto& subKey : subKeys) {
                auto subVal = val[subKey];
                if (subVal.isString()) {
                    mapping[subKey] = subVal.get<std::string>();
                }
            }
            atlas.labels[key] = std::move(mapping);
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
    std::stringstream buffer;
    buffer << file.rdbuf();

    try {
        Json j = Json::parse(buffer.str());
        return loadFromJson(j);
    } catch (const std::exception& e) {
        std::cerr << "[AnimationManifest] Failed to parse " << path << ": " << e.what() << '\n';
        return AnimationAtlas{};
    }
}
