#include "components/LivesComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "systems/HUDSystem.hpp"

#include <SFML/Window/VideoMode.hpp>
#include <gtest/gtest.h>

class HUDSystemFixture : public ::testing::Test
{
  protected:
    Window window{sf::VideoMode({200u, 200u}), "HUD Test", false};
    FontManager fonts;
    TextureManager textures;
    HUDSystem system{window, fonts, textures};
    Registry registry;
};

TEST_F(HUDSystemFixture, UpdatesScoreContent)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<ScoreComponent>(e, ScoreComponent::create(123));

    system.update(registry, 0.0F);

    EXPECT_EQ(text.content, "Score: 123");
}

TEST_F(HUDSystemFixture, UpdatesLivesContent)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<LivesComponent>(e, LivesComponent::create(2, 5));

    system.update(registry, 0.0F);

    EXPECT_EQ(text.content, "Lives:");
}

TEST_F(HUDSystemFixture, PrefersScoreOverLivesWhenBothPresent)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<ScoreComponent>(e, ScoreComponent::create(10));
    registry.emplace<LivesComponent>(e, LivesComponent::create(1, 3));

    system.update(registry, 0.0F);

    EXPECT_EQ(text.content, "Score: 10");
}

TEST_F(HUDSystemFixture, LivesStringHandlesZeroMax)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<LivesComponent>(e, LivesComponent::create(0, 0));

    system.update(registry, 0.0F);

    EXPECT_EQ(text.content, "Lives:");
}

TEST_F(HUDSystemFixture, LivesStringAllowsCurrentAboveMax)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<LivesComponent>(e, LivesComponent::create(5, 3));

    system.update(registry, 0.0F);

    EXPECT_EQ(text.content, "Lives:");
}

TEST_F(HUDSystemFixture, MixedScoreAndLivesEntities)
{
    EntityId scoreE = registry.createEntity();
    registry.emplace<TransformComponent>(scoreE, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TextComponent>(scoreE, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<ScoreComponent>(scoreE, ScoreComponent::create(77));

    EntityId livesE = registry.createEntity();
    registry.emplace<TransformComponent>(livesE, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TextComponent>(livesE, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<LivesComponent>(livesE, LivesComponent::create(2, 4));

    system.update(registry, 0.0F);

    auto& tScore = registry.get<TextComponent>(scoreE);
    auto& tLives = registry.get<TextComponent>(livesE);
    EXPECT_EQ(tScore.content, "Score: 77");
    EXPECT_EQ(tLives.content, "Lives:");
}

TEST_F(HUDSystemFixture, UpdatesAfterLivesChange)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text  = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    auto& lives = registry.emplace<LivesComponent>(e, LivesComponent::create(3, 3));

    system.update(registry, 0.0F);
    EXPECT_EQ(text.content, "Lives:");

    lives.current = 1;
    system.update(registry, 0.0F);
    EXPECT_EQ(text.content, "Lives:");
}

TEST_F(HUDSystemFixture, UpdatesAfterScoreChange)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text  = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    auto& score = registry.emplace<ScoreComponent>(e, ScoreComponent::create(0));

    system.update(registry, 0.0F);
    EXPECT_EQ(text.content, "Score: 0");

    score.add(42);
    system.update(registry, 0.0F);
    EXPECT_EQ(text.content, "Score: 42");
}
