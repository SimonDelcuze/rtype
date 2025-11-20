#include "components/AnimationComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/AnimationSystem.hpp"

#include <gtest/gtest.h>

class AnimationSystemTest : public ::testing::Test
{
  protected:
    Registry registry;
    AnimationSystem system;

    EntityId createAnimatedEntity(std::uint32_t frameCount, float frameTime, bool loop = true)
    {
        EntityId entity = registry.createEntity();
        registry.emplace<AnimationComponent>(entity, AnimationComponent::create(frameCount, frameTime, loop));
        auto& sprite = registry.emplace<SpriteComponent>(entity);
        sprite.setFrameSize(32, 32, frameCount);
        return entity;
    }
};

TEST_F(AnimationSystemTest, AdvancesFrameAfterFrameTime)
{
    EntityId entity = createAnimatedEntity(4, 0.1F);
    auto& anim      = registry.get<AnimationComponent>(entity);

    EXPECT_EQ(anim.currentFrame, 0);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 1);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 2);
}

TEST_F(AnimationSystemTest, LoopsAnimation)
{
    EntityId entity = createAnimatedEntity(3, 0.1F, true);
    auto& anim      = registry.get<AnimationComponent>(entity);

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
    EntityId entity = createAnimatedEntity(3, 0.1F, false);
    auto& anim      = registry.get<AnimationComponent>(entity);

    system.update(registry, 0.1F);
    system.update(registry, 0.1F);
    system.update(registry, 0.1F);

    EXPECT_EQ(anim.currentFrame, 2);
    EXPECT_TRUE(anim.finished);
    EXPECT_FALSE(anim.playing);
}

TEST_F(AnimationSystemTest, ReverseDirection)
{
    EntityId entity   = createAnimatedEntity(3, 0.1F, true);
    auto& anim        = registry.get<AnimationComponent>(entity);
    anim.direction    = AnimationDirection::Reverse;
    anim.currentFrame = 2;

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 1);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 0);

    system.update(registry, 0.1F);
    EXPECT_EQ(anim.currentFrame, 2);
}

TEST_F(AnimationSystemTest, PingPongDirection)
{
    EntityId entity = createAnimatedEntity(3, 0.1F, true);
    auto& anim      = registry.get<AnimationComponent>(entity);
    anim.direction  = AnimationDirection::PingPong;

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

TEST_F(AnimationSystemTest, UpdatesSpriteFrame)
{
    EntityId entity = createAnimatedEntity(4, 0.1F);
    auto& sprite    = registry.get<SpriteComponent>(entity);

    EXPECT_EQ(sprite.getFrame(), 0);

    system.update(registry, 0.1F);
    EXPECT_EQ(sprite.getFrame(), 1);

    system.update(registry, 0.1F);
    EXPECT_EQ(sprite.getFrame(), 2);
}

TEST_F(AnimationSystemTest, DoesNotUpdatePausedAnimation)
{
    EntityId entity = createAnimatedEntity(4, 0.1F);
    auto& anim      = registry.get<AnimationComponent>(entity);
    auto& sprite    = registry.get<SpriteComponent>(entity);
    anim.pause();

    system.update(registry, 0.5F);

    EXPECT_EQ(anim.currentFrame, 0);
    EXPECT_EQ(sprite.getFrame(), 0);
}

TEST_F(AnimationSystemTest, DoesNotUpdateFinishedAnimation)
{
    EntityId entity = createAnimatedEntity(4, 0.1F);
    auto& anim      = registry.get<AnimationComponent>(entity);
    auto& sprite    = registry.get<SpriteComponent>(entity);
    anim.finished   = true;

    system.update(registry, 0.5F);

    EXPECT_EQ(anim.currentFrame, 0);
    EXPECT_EQ(sprite.getFrame(), 0);
}

TEST_F(AnimationSystemTest, SkipsDeadEntities)
{
    EntityId entity = createAnimatedEntity(4, 0.1F);
    registry.destroyEntity(entity);

    system.update(registry, 0.5F);
    // No crash = success
}

TEST_F(AnimationSystemTest, SkipsEntitiesWithoutSpriteComponent)
{
    EntityId entity = registry.createEntity();
    registry.emplace<AnimationComponent>(entity, AnimationComponent::create(4, 0.1F));

    system.update(registry, 0.5F);
    // No crash = success, animation not updated because no sprite
}

TEST_F(AnimationSystemTest, AccumulatesTime)
{
    EntityId entity = createAnimatedEntity(4, 0.1F);
    auto& anim      = registry.get<AnimationComponent>(entity);

    system.update(registry, 0.05F);
    EXPECT_EQ(anim.currentFrame, 0);

    system.update(registry, 0.05F);
    EXPECT_EQ(anim.currentFrame, 1);
}

TEST_F(AnimationSystemTest, HandlesMultipleFramesPerUpdate)
{
    EntityId entity = createAnimatedEntity(4, 0.1F);
    auto& anim      = registry.get<AnimationComponent>(entity);
    auto& sprite    = registry.get<SpriteComponent>(entity);

    system.update(registry, 0.35F);

    EXPECT_EQ(anim.currentFrame, 3);
    EXPECT_EQ(sprite.getFrame(), 3);
}
