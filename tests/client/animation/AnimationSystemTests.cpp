#include "components/AnimationComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/AnimationSystem.hpp"

#include <gtest/gtest.h>

class AnimationSystemTest : public ::testing::Test
{
  protected:
    Registry registry;
    AnimationSystem system;
    std::vector<std::tuple<EntityId, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t>> callbacks;

    void SetUp() override
    {
        callbacks.clear();
        system.setSpriteRectCallback([this](EntityId id, std::uint32_t x, std::uint32_t y, std::uint32_t w,
                                            std::uint32_t h) { callbacks.emplace_back(id, x, y, w, h); });
    }
};

TEST_F(AnimationSystemTest, AdvancesFrameAfterFrameTime)
{
    EntityId entity  = registry.createEntity();
    auto& anim       = registry.emplace<AnimationComponent>(entity, AnimationComponent::create(4, 0.1F));
    anim.frameWidth  = 32;
    anim.frameHeight = 32;
    anim.columns     = 4;

    EXPECT_EQ(anim.currentFrame, 0);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 1);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 2);
}

TEST_F(AnimationSystemTest, LoopsAnimation)
{
    EntityId entity  = registry.createEntity();
    auto& anim       = registry.emplace<AnimationComponent>(entity, AnimationComponent::create(3, 0.1F, true));
    anim.frameWidth  = 32;
    anim.frameHeight = 32;
    anim.columns     = 3;

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 1);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 2);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 0);
    EXPECT_FALSE(anim.finished);
}

TEST_F(AnimationSystemTest, StopsAtEndWithoutLoop)
{
    EntityId entity  = registry.createEntity();
    auto& anim       = registry.emplace<AnimationComponent>(entity, AnimationComponent::create(3, 0.1F, false));
    anim.frameWidth  = 32;
    anim.frameHeight = 32;
    anim.columns     = 3;

    system.update(registry, 0.1F);
    system.update(registry, 0.1F);
    system.update(registry, 0.1F);

    EXPECT_EQ(anim.currentFrame, 2);
    EXPECT_TRUE(anim.finished);
    EXPECT_FALSE(anim.playing);
}

TEST_F(AnimationSystemTest, ReverseDirection)
{
    EntityId entity   = registry.createEntity();
    auto& anim        = registry.emplace<AnimationComponent>(entity, AnimationComponent::create(3, 0.1F, true));
    anim.direction    = AnimationDirection::Reverse;
    anim.currentFrame = 2;
    anim.frameWidth   = 32;
    anim.frameHeight  = 32;
    anim.columns      = 3;

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 1);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 0);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 2);
}

TEST_F(AnimationSystemTest, PingPongDirection)
{
    EntityId entity  = registry.createEntity();
    auto& anim       = registry.emplace<AnimationComponent>(entity, AnimationComponent::create(3, 0.1F, true));
    anim.direction   = AnimationDirection::PingPong;
    anim.frameWidth  = 32;
    anim.frameHeight = 32;
    anim.columns     = 3;

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 1);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 2);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 1);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 0);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 1);
}

TEST_F(AnimationSystemTest, CallbackCalledWithCorrectCoordinates)
{
    EntityId entity  = registry.createEntity();
    auto& anim       = registry.emplace<AnimationComponent>(entity, AnimationComponent::create(4, 0.1F));
    anim.frameWidth  = 32;
    anim.frameHeight = 32;
    anim.columns     = 4;

    system.update(registry, 0.1F);

    ASSERT_EQ(callbacks.size(), 1);
    auto& [id, x, y, w, h] = callbacks[0];
    EXPECT_EQ(id, entity);
    EXPECT_EQ(x, 32);
    EXPECT_EQ(y, 0);
    EXPECT_EQ(w, 32);
    EXPECT_EQ(h, 32);
}

TEST_F(AnimationSystemTest, CallbackWithMultipleRows)
{
    EntityId entity  = registry.createEntity();
    auto& anim       = registry.emplace<AnimationComponent>(entity, AnimationComponent::fromIndices({4}, 0.1F));
    anim.frameWidth  = 32;
    anim.frameHeight = 32;
    anim.columns     = 2;

    anim.currentFrame = 0;
    system.update(registry, 0.1F);

    ASSERT_EQ(callbacks.size(), 1);
    auto& [id, x, y, w, h] = callbacks[0];
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 64);
}

TEST_F(AnimationSystemTest, DoesNotUpdatePausedAnimation)
{
    EntityId entity  = registry.createEntity();
    auto& anim       = registry.emplace<AnimationComponent>(entity, AnimationComponent::create(4, 0.1F));
    anim.frameWidth  = 32;
    anim.frameHeight = 32;
    anim.columns     = 4;
    anim.pause();

    system.update(registry, 0.5F);

    EXPECT_EQ(anim.currentFrame, 0);
    EXPECT_TRUE(callbacks.empty());
}

TEST_F(AnimationSystemTest, DoesNotUpdateFinishedAnimation)
{
    EntityId entity  = registry.createEntity();
    auto& anim       = registry.emplace<AnimationComponent>(entity, AnimationComponent::create(4, 0.1F));
    anim.frameWidth  = 32;
    anim.frameHeight = 32;
    anim.columns     = 4;
    anim.finished    = true;

    system.update(registry, 0.5F);

    EXPECT_EQ(anim.currentFrame, 0);
    EXPECT_TRUE(callbacks.empty());
}

TEST_F(AnimationSystemTest, SkipsDeadEntities)
{
    EntityId entity = registry.createEntity();
    registry.emplace<AnimationComponent>(entity, AnimationComponent::create(4, 0.1F));
    registry.destroyEntity(entity);

    system.update(registry, 0.5F);

    EXPECT_TRUE(callbacks.empty());
}

TEST_F(AnimationSystemTest, AccumulatesTime)
{
    EntityId entity  = registry.createEntity();
    auto& anim       = registry.emplace<AnimationComponent>(entity, AnimationComponent::create(4, 0.1F));
    anim.frameWidth  = 32;
    anim.frameHeight = 32;
    anim.columns     = 4;

    system.update(registry, 0.05F);
    EXPECT_EQ(anim.currentFrame, 0);

    system.update(registry, 0.05F);
    EXPECT_EQ(anim.currentFrame, 1);
}

TEST_F(AnimationSystemTest, HandlesMultipleFramesPerUpdate)
{
    EntityId entity  = registry.createEntity();
    auto& anim       = registry.emplace<AnimationComponent>(entity, AnimationComponent::create(4, 0.1F));
    anim.frameWidth  = 32;
    anim.frameHeight = 32;
    anim.columns     = 4;

    system.update(registry, 0.35F);

    EXPECT_EQ(anim.currentFrame, 3);
    EXPECT_EQ(callbacks.size(), 3);
}
