#include "components/TextComponent.hpp"
#include "ecs/ResetValue.hpp"

#include <gtest/gtest.h>

TEST(TextComponent, DefaultValues)
{
    TextComponent t{};
    EXPECT_EQ(t.fontId, "");
    EXPECT_EQ(t.characterSize, 24u);
    EXPECT_EQ(t.color, Color::White);
    EXPECT_EQ(t.content, "");
    EXPECT_EQ(t.text, nullptr);
}

TEST(TextComponent, CreateHelper)
{
    Color c         = Color::Blue;
    TextComponent b = TextComponent::create("arial.ttf", 32, c);

    EXPECT_EQ(b.fontId, "arial.ttf");
    EXPECT_EQ(b.characterSize, 32u);
    EXPECT_EQ(b.color, Color::Blue);
    EXPECT_EQ(b.content, "");
    EXPECT_EQ(b.text, nullptr);
}

TEST(TextComponent, CopyPreservesValues)
{
    auto original    = TextComponent::create("hud", 18, Color::Green);
    original.content = "Hello";
    TextComponent copy(original);

    EXPECT_EQ(copy.fontId, "hud");
    EXPECT_EQ(copy.characterSize, 18u);
    EXPECT_EQ(copy.color, Color::Green);
    EXPECT_EQ(copy.content, "Hello");
    EXPECT_EQ(copy.text, original.text);
}

TEST(TextComponent, AssignmentPreservesValues)
{
    auto a          = TextComponent::create("a", 12, Color::Blue);
    a.content       = "A";
    TextComponent b = TextComponent::create("b", 14, Color::Red);
    b.content       = "B";

    b = a;

    EXPECT_EQ(b.fontId, "a");
    EXPECT_EQ(b.characterSize, 12u);
    EXPECT_EQ(b.color, Color::Blue);
    EXPECT_EQ(b.content, "A");
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
    t.color = Color::Black;
    EXPECT_EQ(t.color, Color::Black);
}

TEST(TextComponent, OptionalTextInitiallyEmpty)
{
    TextComponent t{};
    EXPECT_EQ(t.text, nullptr);
}

TEST(TextComponent, OptionalTextClearedWithReset)
{
    TextComponent t{};
    resetValue<TextComponent>(t);

    resetValue<TextComponent>(t);

    EXPECT_EQ(t.text, nullptr);
    EXPECT_EQ(t.content, "");
    EXPECT_EQ(t.fontId, "");
}

TEST(TextComponent, ResetRestoresDefaults)
{
    TextComponent t = TextComponent::create("hud", 30, Color::Yellow);
    t.content       = "data";
    resetValue<TextComponent>(t);

    EXPECT_EQ(t.fontId, "");
    EXPECT_EQ(t.characterSize, 24u);
    EXPECT_EQ(t.color, Color::White);
    EXPECT_EQ(t.content, "");
    EXPECT_EQ(t.text, nullptr);
}

TEST(TextComponent, MultipleInstancesIndependent)
{
    auto a    = TextComponent::create("a", 10, Color::Red);
    auto b    = TextComponent::create("b", 20, Color::Blue);
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
