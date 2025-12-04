#include "animation/AnimationRegistry.hpp"

void AnimationRegistry::registerClip(AnimationClip clip)
{
    clips_[clip.id] = std::move(clip);
}

const AnimationClip* AnimationRegistry::get(const std::string& id) const
{
    auto it = clips_.find(id);
    if (it == clips_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool AnimationRegistry::has(const std::string& id) const
{
    return clips_.find(id) != clips_.end();
}
