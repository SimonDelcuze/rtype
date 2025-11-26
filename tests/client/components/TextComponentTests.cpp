#include "components/TextComponent.hpp"
#include "ecs/ResetValue.hpp"

#include <SFML/Graphics/Font.hpp>
#include <gtest/gtest.h>

TEST(TextComponent, DefaultValues)
{
    TextComponent t{};
    EXPECT_EQ(t.fontId, "");
    EXPECT_EQ(t.characterSize, 24u);
    EXPECT_EQ(t.color, sf::Color::White);
    EXPECT_EQ(t.content, "");
    EXPECT_FALSE(t.text.has_value());
}

TEST(TextComponent, CreateSetsFields)
{
    auto t = TextComponent::create("main", 32, sf::Color::Red);
    EXPECT_EQ(t.fontId, "main");
    EXPECT_EQ(t.characterSize, 32u);
    EXPECT_EQ(t.color, sf::Color::Red);
    EXPECT_EQ(t.content, "");
    EXPECT_FALSE(t.text.has_value());
}

TEST(TextComponent, CopyPreservesValues)
{
    auto original = TextComponent::create("hud", 18, sf::Color::Green);
    original.content = "Hello";
    TextComponent copy(original);

    EXPECT_EQ(copy.fontId, "hud");
    EXPECT_EQ(copy.characterSize, 18u);
    EXPECT_EQ(copy.color, sf::Color::Green);
    EXPECT_EQ(copy.content, "Hello");
    EXPECT_EQ(copy.text.has_value(), original.text.has_value());
}

TEST(TextComponent, AssignmentPreservesValues)
{
    auto a = TextComponent::create("a", 12, sf::Color::Blue);
    a.content = "A";
    TextComponent b = TextComponent::create("b", 14, sf::Color::Red);
    b.content        = "B";

    b = a;

    EXPECT_EQ(b.fontId, "a");
    EXPECT_EQ(b.characterSize, 12u);
    EXPECT_EQ(b.color, sf::Color::Blue);
    EXPECT_EQ(b.content, "A");
}

TEST(TextComponent, MutateContent)
{
    TextComponent t{};
    t.content = "Score: 100";
    EXPECT_EQ(t.content, "Score: 100");
    t.content = "Score: 200";
    EXPECT_EQ(t.content, "Score: 200");
}

TEST(TextComponent, MutateFontId)
{
    TextComponent t{};
    t.fontId = "primary";
    EXPECT_EQ(t.fontId, "primary");
    t.fontId = "secondary";
    EXPECT_EQ(t.fontId, "secondary");
}

TEST(TextComponent, MutateCharacterSize)
{
    TextComponent t{};
    t.characterSize = 48;
    EXPECT_EQ(t.characterSize, 48u);
}

TEST(TextComponent, MutateColor)
{
    TextComponent t{};
    t.color = sf::Color::Black;
    EXPECT_EQ(t.color, sf::Color::Black);
}

TEST(TextComponent, OptionalTextInitiallyEmpty)
{
    TextComponent t{};
    EXPECT_FALSE(t.text.has_value());
}

TEST(TextComponent, OptionalTextEmplace)
{
    TextComponent t{};
    sf::Font dummy;
    t.text.emplace(dummy, "hi", 16);

    ASSERT_TRUE(t.text.has_value());
    EXPECT_EQ(t.text->getString(), "hi");
    EXPECT_EQ(t.text->getCharacterSize(), 16u);
}

TEST(TextComponent, OptionalTextClearedWithReset)
{
    TextComponent t{};
    sf::Font dummy;
    t.text.emplace(dummy, "temp", 12);
    ASSERT_TRUE(t.text.has_value());

    resetValue<TextComponent>(t);

    EXPECT_FALSE(t.text.has_value());
    EXPECT_EQ(t.content, "");
    EXPECT_EQ(t.fontId, "");
}

TEST(TextComponent, ResetRestoresDefaults)
{
    TextComponent t = TextComponent::create("hud", 30, sf::Color::Yellow);
    t.content       = "data";
    resetValue<TextComponent>(t);

    EXPECT_EQ(t.fontId, "");
    EXPECT_EQ(t.characterSize, 24u);
    EXPECT_EQ(t.color, sf::Color::White);
    EXPECT_EQ(t.content, "");
    EXPECT_FALSE(t.text.has_value());
}

TEST(TextComponent, MultipleInstancesIndependent)
{
    auto a = TextComponent::create("a", 10, sf::Color::Red);
    auto b = TextComponent::create("b", 20, sf::Color::Blue);
    a.content = "X";
    b.content = "Y";

    EXPECT_EQ(a.content, "X");
    EXPECT_EQ(b.content, "Y");
    EXPECT_EQ(a.fontId, "a");
    EXPECT_EQ(b.fontId, "b");
}

TEST(TextComponent, ContentCanBeClearedManually)
{
    TextComponent t{};
    t.content = "Test";
    t.content.clear();
    EXPECT_EQ(t.content, "");
}

TEST(TextComponent, CharacterSizeSupportsLargeValue)
{
    TextComponent t{};
    t.characterSize = 128;
    EXPECT_EQ(t.characterSize, 128u);
}
