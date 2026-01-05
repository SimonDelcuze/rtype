#pragma once

#include "animation/AnimationRegistry.hpp"

#include "json/Json.hpp"
#include <string>
#include <unordered_map>

struct AnimationAtlas
{
    AnimationRegistry clips;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> labels;
};

class AnimationManifest
{
  public:
    static AnimationAtlas loadFromFile(const std::string& path);
    static AnimationAtlas loadFromJson(const rtype::Json& j);
};
