#include "components/LivesComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/TagComponent.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "network/RoomType.hpp"
#include "systems/HUDSystem.hpp"

#include <SFML/Window/VideoMode.hpp>
#include <gtest/gtest.h>

class HUDSystemFixture : public ::testing::Test
{
  protected:
    Window window{{200u, 200u}, "HUD Test"};
    FontManager fonts;
    TextureManager textures;
    HUDSystem system{window, fonts, textures, 0, RoomType::Quickplay};
    Registry registry;
};

TEST_F(HUDSystemFixture, UpdatesScoreContent)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("", 20, Color::White));
    registry.emplace<TagComponent>(e, TagComponent::create(EntityTag::Player));
    registry.emplace<ScoreComponent>(e, ScoreComponent::create(123));

    system.update(registry, 0.0F);

    EXPECT_EQ(text.content, "SCORE 0000123");
}

TEST_F(HUDSystemFixture, PrefersScoreOverLivesWhenBothPresent)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("", 20, Color::White));
    registry.emplace<TagComponent>(e, TagComponent::create(EntityTag::Player));
    registry.emplace<ScoreComponent>(e, ScoreComponent::create(10));
    registry.emplace<LivesComponent>(e, LivesComponent::create(1, 3));

    system.update(registry, 0.0F);

    EXPECT_EQ(text.content, "SCORE 0000010");
}

TEST_F(HUDSystemFixture, UpdatesAfterScoreChange)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text  = registry.emplace<TextComponent>(e, TextComponent::create("", 20, Color::White));
    registry.emplace<TagComponent>(e, TagComponent::create(EntityTag::Player));
    auto& score = registry.emplace<ScoreComponent>(e, ScoreComponent::create(50));

    system.update(registry, 0.0F);
    EXPECT_EQ(text.content, "SCORE 0000050");

    score.value = 999;
    system.update(registry, 0.0F);
    EXPECT_EQ(text.content, "SCORE 0000999");
}
