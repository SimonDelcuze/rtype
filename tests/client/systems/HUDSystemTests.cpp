#include "components/LivesComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/Window.hpp"
#include "systems/HUDSystem.hpp"

#include <SFML/Window/VideoMode.hpp>
#include <gtest/gtest.h>

class HUDSystemFixture : public ::testing::Test
{
  protected:
    Window window{sf::VideoMode({200u, 200u}), "HUD Test", false};
    FontManager fonts;
    HUDSystem system{window, fonts};
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

    EXPECT_EQ(text.content, "Lives: 2/5");
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

TEST_F(HUDSystemFixture, SkipsEntityWithoutTransform)
{
    EntityId e = registry.createEntity();
    registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<ScoreComponent>(e, ScoreComponent::create(50));

    system.update(registry, 0.0F);

    // Not in the view, so untouched
    EXPECT_EQ(registry.get<TextComponent>(e).content, "");
}

TEST_F(HUDSystemFixture, SkipsDeadEntity)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<ScoreComponent>(e, ScoreComponent::create(20));
    registry.destroyEntity(e);

    system.update(registry, 0.0F);

    EXPECT_FALSE(registry.has<TextComponent>(e));
}

TEST_F(HUDSystemFixture, NoFontIdLeavesTextOptionalEmpty)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<ScoreComponent>(e, ScoreComponent::create(5));

    system.update(registry, 0.0F);

    EXPECT_FALSE(text.text.has_value());
}

TEST_F(HUDSystemFixture, UnknownFontIdLeavesTextOptionalEmpty)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("missing", 20, sf::Color::White));
    registry.emplace<ScoreComponent>(e, ScoreComponent::create(5));

    system.update(registry, 0.0F);

    EXPECT_FALSE(text.text.has_value());
}

TEST_F(HUDSystemFixture, UpdatesContentEachFrame)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text  = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    auto& score = registry.emplace<ScoreComponent>(e, ScoreComponent::create(0));

    system.update(registry, 0.0F);
    EXPECT_EQ(text.content, "Score: 0");

    score.value = 900;
    system.update(registry, 0.0F);
    EXPECT_EQ(text.content, "Score: 900");
}

TEST_F(HUDSystemFixture, LivesStringHandlesZeroMax)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<LivesComponent>(e, LivesComponent::create(0, 0));

    system.update(registry, 0.0F);

    EXPECT_EQ(text.content, "Lives: 0/0");
}

TEST_F(HUDSystemFixture, LivesStringAllowsCurrentAboveMax)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<LivesComponent>(e, LivesComponent::create(5, 3));

    system.update(registry, 0.0F);

    EXPECT_EQ(text.content, "Lives: 5/3");
}

TEST_F(HUDSystemFixture, MultipleScoreEntitiesEachUpdated)
{
    EntityId e1 = registry.createEntity();
    registry.emplace<TransformComponent>(e1, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TextComponent>(e1, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<ScoreComponent>(e1, ScoreComponent::create(10));

    EntityId e2 = registry.createEntity();
    registry.emplace<TransformComponent>(e2, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TextComponent>(e2, TextComponent::create("", 20, sf::Color::White));
    registry.emplace<ScoreComponent>(e2, ScoreComponent::create(20));

    system.update(registry, 0.0F);

    auto& t1 = registry.get<TextComponent>(e1);
    auto& t2 = registry.get<TextComponent>(e2);
    EXPECT_EQ(t1.content, "Score: 10");
    EXPECT_EQ(t2.content, "Score: 20");
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
    EXPECT_EQ(tLives.content, "Lives: 2/4");
}

TEST_F(HUDSystemFixture, TextWithoutScoreOrLivesUnchanged)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text   = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    text.content = "Static";

    system.update(registry, 0.0F);

    EXPECT_EQ(text.content, "Static");
}

TEST_F(HUDSystemFixture, DoesNotThrowWithNoEntities)
{
    EXPECT_NO_THROW(system.update(registry, 0.0F));
}

TEST_F(HUDSystemFixture, HandlesMultipleUpdatesWithoutFont)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text = registry.emplace<TextComponent>(e, TextComponent::create("missing", 20, sf::Color::White));
    registry.emplace<ScoreComponent>(e, ScoreComponent::create(1));

    for (int i = 0; i < 3; ++i) {
        system.update(registry, 0.016F);
    }

    EXPECT_EQ(text.content, "Score: 1");
    EXPECT_FALSE(text.text.has_value());
}

TEST_F(HUDSystemFixture, UpdatesAfterLivesChange)
{
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    auto& text  = registry.emplace<TextComponent>(e, TextComponent::create("", 20, sf::Color::White));
    auto& lives = registry.emplace<LivesComponent>(e, LivesComponent::create(3, 3));

    system.update(registry, 0.0F);
    EXPECT_EQ(text.content, "Lives: 3/3");

    lives.current = 1;
    system.update(registry, 0.0F);
    EXPECT_EQ(text.content, "Lives: 1/3");
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
