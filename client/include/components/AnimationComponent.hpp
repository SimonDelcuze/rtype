#pragma once

#include <cstdint>
#include <vector>

enum class AnimationDirection : std::uint8_t
{
    Forward,
    Reverse,
    PingPong
};

struct AnimationComponent
{
    std::vector<std::uint32_t> frameIndices;
    float frameTime              = 0.1F;
    float elapsedTime            = 0.0F;
    std::uint32_t currentFrame   = 0;
    bool loop                    = true;
    bool playing                 = true;
    bool finished                = false;
    AnimationDirection direction = AnimationDirection::Forward;
    bool pingPongReverse         = false;

    std::uint32_t frameWidth  = 0;
    std::uint32_t frameHeight = 0;
    std::uint32_t columns     = 1;

    static AnimationComponent create(std::uint32_t frameCount, float frameTime, bool loop = true);
    static AnimationComponent fromIndices(std::vector<std::uint32_t> indices, float frameTime, bool loop = true);

    void play();
    void pause();
    void stop();
    void reset();
    std::uint32_t getCurrentFrameIndex() const;
};
