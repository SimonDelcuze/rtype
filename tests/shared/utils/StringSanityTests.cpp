#include "utils/StringSanity.hpp"

#include <gtest/gtest.h>

using namespace rtype::utils;

TEST(StringSanityTests, IsSafeMessage)
{
    EXPECT_TRUE(isSafeChatMessage("Hello world!"));
    EXPECT_TRUE(isSafeChatMessage("How are you?"));
    EXPECT_TRUE(isSafeChatMessage("1234567890"));

    // Unsafe characters
    EXPECT_FALSE(isSafeChatMessage("<script>"));
    EXPECT_FALSE(isSafeChatMessage("Hello <there>"));
    EXPECT_FALSE(isSafeChatMessage("Fish & Chips"));
    EXPECT_FALSE(isSafeChatMessage("He said \"Hello\""));
}

TEST(StringSanityTests, SanitizeMessage)
{
    EXPECT_EQ(sanitizeChatMessage("Hello world!"), "Hello world!");
    EXPECT_EQ(sanitizeChatMessage("<script>alert(1)</script>"), "scriptalert(1)/script");
    EXPECT_EQ(sanitizeChatMessage("Fish & Chips"), "Fish  Chips");

    // Non-printable characters
    std::string complex = "Hello";
    complex += static_cast<char>(7); // Bell
    complex += "World";
    EXPECT_EQ(sanitizeChatMessage(complex), "HelloWorld");
}
