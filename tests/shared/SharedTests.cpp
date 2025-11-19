#include "Test.hpp"

#include <gtest/gtest.h>
#include <sstream>

TEST(Shared, HelloPrintsMessage)
{
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    shared_hello("unit-test");

    std::cout.rdbuf(old);

    const std::string output = buffer.str();
    EXPECT_NE(output.find("[shared] hello from unit-test"), std::string::npos);
}
