#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <string>
#include <unordered_map>
#include <vector>

struct AnimationFrame
{
    int x      = 0;
    int y      = 0;
    int width  = 0;
    int height = 0;
};

struct AnimationClip
{
    std::string id;
    float frameTime = 0.1F;
    bool loop       = true;
    std::vector<AnimationFrame> frames;
};

class AnimationRegistry
{
  public:
    void registerClip(AnimationClip clip);
    const AnimationClip* get(const std::string& id) const;
    bool has(const std::string& id) const;

  private:
    std::unordered_map<std::string, AnimationClip> clips_;
};
