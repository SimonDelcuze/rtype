#include <gtest/gtest.h>

#include "Main.hpp"

#include <sstream>

namespace {
std::string capture_client_output()
{
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    run_client();
    std::cout.rdbuf(old);
    return buffer.str();
}
} // namespace

TEST(Client, RunsAndPrints)
{
    const std::string output = capture_client_output();
    EXPECT_NE(output.find("client"), std::string::npos);
    EXPECT_NE(output.find("[shared] hello from client"), std::string::npos);
}
