#include "components/AnimationComponent.hpp"

AnimationComponent AnimationComponent::create(std::uint32_t frameCount, float frameTime, bool loop)
{
    AnimationComponent anim;
    anim.frameTime = frameTime;
    anim.loop      = loop;
    anim.frameIndices.reserve(frameCount);
    for (std::uint32_t i = 0; i < frameCount; ++i) {
        anim.frameIndices.push_back(i);
    }
    return anim;
}

AnimationComponent AnimationComponent::fromIndices(std::vector<std::uint32_t> indices, float frameTime, bool loop)
{
    AnimationComponent anim;
    anim.frameIndices = std::move(indices);
    anim.frameTime    = frameTime;
    anim.loop         = loop;
    return anim;
}

void AnimationComponent::play()
{
    playing  = true;
    finished = false;
}

void AnimationComponent::pause()
{
    playing = false;
}

void AnimationComponent::stop()
{
    playing      = false;
    finished     = true;
    currentFrame = 0;
    elapsedTime  = 0.0F;
}

void AnimationComponent::reset()
{
    currentFrame    = 0;
    elapsedTime     = 0.0F;
    finished        = false;
    pingPongReverse = false;
}

std::uint32_t AnimationComponent::getCurrentFrameIndex() const
{
    if (frameIndices.empty()) {
        return 0;
    }
    return frameIndices[currentFrame];
}
