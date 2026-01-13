#include "components/LivesComponent.hpp"
#include "components/OwnershipComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "components/TagComponent.hpp"
#include "ecs/Registry.hpp"
#include "events/EventBus.hpp"
#include "events/GameEvents.hpp"
#include "network/RoomType.hpp"
#include "systems/GameOverSystem.hpp"

#include <gtest/gtest.h>

TEST(GameOverSystemTest, EmitsEventOnZeroLives)
{
    EventBus eventBus;
    GameOverSystem system(eventBus, 0, RoomType::Quickplay);
    Registry registry;

    bool eventEmitted = false;
    eventBus.subscribe<GameOverEvent>([&](const GameOverEvent& event) {
        eventEmitted = true;
        EXPECT_FALSE(event.victory);
        ASSERT_EQ(event.playerScores.size(), 1);
        EXPECT_EQ(event.playerScores[0].score, 100);
    });

    EntityId player = registry.createEntity();
    registry.emplace<TagComponent>(player, TagComponent::create(EntityTag::Player));
    registry.emplace<OwnershipComponent>(player, OwnershipComponent::create(0));
    registry.emplace<LivesComponent>(player, LivesComponent::create(0, 3));
    registry.emplace<ScoreComponent>(player, ScoreComponent::create(100));

    system.update(registry, 0.16f);
    eventBus.process();
    EXPECT_TRUE(eventEmitted);
}

TEST(GameOverSystemTest, DoesNotEmitIfAlive)
{
    EventBus eventBus;
    GameOverSystem system(eventBus, 0, RoomType::Quickplay);
    Registry registry;

    bool eventEmitted = false;
    eventBus.subscribe<GameOverEvent>([&](const GameOverEvent&) { eventEmitted = true; });

    EntityId player = registry.createEntity();
    registry.emplace<TagComponent>(player, TagComponent::create(EntityTag::Player));
    registry.emplace<OwnershipComponent>(player, OwnershipComponent::create(0));
    registry.emplace<LivesComponent>(player, LivesComponent::create(3, 3));

    system.update(registry, 0.16f);
    EXPECT_FALSE(eventEmitted);
}
