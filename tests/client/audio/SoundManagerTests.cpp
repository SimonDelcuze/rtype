#include "audio/SoundManager.hpp"

#include <gtest/gtest.h>

TEST(SoundManagerTests, DefaultConstruction)
{
    SoundManager manager;
    EXPECT_FALSE(manager.has("nonexistent"));
}

TEST(SoundManagerTests, HasReturnsFalseForNonexistent)
{
    SoundManager manager;
    EXPECT_FALSE(manager.has("missing"));
}

TEST(SoundManagerTests, GetReturnsNullptrForNonexistent)
{
    SoundManager manager;
    EXPECT_EQ(manager.get("missing"), nullptr);
}

TEST(SoundManagerTests, LoadNonexistentFileThrows)
{
    SoundManager manager;
    EXPECT_THROW(manager.load("test", "nonexistent_file.wav"), std::runtime_error);
    EXPECT_FALSE(manager.has("test"));
}

TEST(SoundManagerTests, RemoveNonexistentDoesNotCrash)
{
    SoundManager manager;
    manager.remove("nonexistent");
    EXPECT_FALSE(manager.has("nonexistent"));
}

TEST(SoundManagerTests, ClearEmptyManagerDoesNotCrash)
{
    SoundManager manager;
    manager.clear();
    EXPECT_FALSE(manager.has("anything"));
}
