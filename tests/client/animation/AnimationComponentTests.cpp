#include "components/AnimationComponent.hpp"

#include <gtest/gtest.h>

TEST(AnimationComponent, CreateWithFrameCount)
{
    auto anim = AnimationComponent::create(8, 0.1F);

    EXPECT_EQ(anim.frameIndices.size(), 8);
    EXPECT_FLOAT_EQ(anim.frameTime, 0.1F);
    EXPECT_TRUE(anim.loop);
    EXPECT_TRUE(anim.playing);
    EXPECT_FALSE(anim.finished);
    EXPECT_EQ(anim.currentFrame, 0);

    for (std::uint32_t i = 0; i < 8; ++i) {
        EXPECT_EQ(anim.frameIndices[i], i);
    }
}

TEST(AnimationComponent, CreateWithoutLoop)
{
    auto anim = AnimationComponent::create(4, 0.2F, false);

    EXPECT_FALSE(anim.loop);
}

TEST(AnimationComponent, FromIndices)
{
    auto anim = AnimationComponent::fromIndices({2, 3, 4, 3}, 0.15F);

    EXPECT_EQ(anim.frameIndices.size(), 4);
    EXPECT_EQ(anim.frameIndices[0], 2);
    EXPECT_EQ(anim.frameIndices[1], 3);
    EXPECT_EQ(anim.frameIndices[2], 4);
    EXPECT_EQ(anim.frameIndices[3], 3);
    EXPECT_FLOAT_EQ(anim.frameTime, 0.15F);
}

TEST(AnimationComponent, PlayPauseStop)
{
    auto anim = AnimationComponent::create(4, 0.1F);

    anim.pause();
    EXPECT_FALSE(anim.playing);

    anim.play();
    EXPECT_TRUE(anim.playing);
    EXPECT_FALSE(anim.finished);

    anim.stop();
    EXPECT_FALSE(anim.playing);
    EXPECT_TRUE(anim.finished);
    EXPECT_EQ(anim.currentFrame, 0);
    EXPECT_FLOAT_EQ(anim.elapsedTime, 0.0F);
}

TEST(AnimationComponent, Reset)
{
    auto anim        = AnimationComponent::create(4, 0.1F);
    anim.currentFrame    = 3;
    anim.elapsedTime     = 0.5F;
    anim.finished        = true;
    anim.pingPongReverse = true;

    anim.reset();

    EXPECT_EQ(anim.currentFrame, 0);
    EXPECT_FLOAT_EQ(anim.elapsedTime, 0.0F);
    EXPECT_FALSE(anim.finished);
    EXPECT_FALSE(anim.pingPongReverse);
}

TEST(AnimationComponent, GetCurrentFrameIndex)
{
    auto anim = AnimationComponent::fromIndices({5, 10, 15}, 0.1F);

    anim.currentFrame = 0;
    EXPECT_EQ(anim.getCurrentFrameIndex(), 5);

    anim.currentFrame = 1;
    EXPECT_EQ(anim.getCurrentFrameIndex(), 10);

    anim.currentFrame = 2;
    EXPECT_EQ(anim.getCurrentFrameIndex(), 15);
}

TEST(AnimationComponent, GetCurrentFrameIndexEmpty)
{
    AnimationComponent anim;
    anim.frameIndices.clear();

    EXPECT_EQ(anim.getCurrentFrameIndex(), 0);
}

TEST(AnimationComponent, DefaultDirection)
{
    auto anim = AnimationComponent::create(4, 0.1F);

    EXPECT_EQ(anim.direction, AnimationDirection::Forward);
}
