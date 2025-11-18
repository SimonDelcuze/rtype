#include "Main.hpp"

#include <gtest/gtest.h>
#include <sstream>

namespace
{
    std::string capture_server_output()
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        run_server();
        std::cout.rdbuf(old);
        return buffer.str();
    }
} // namespace

TEST(Server, RunsAndPrints)
{
    const std::string output = capture_server_output();
    EXPECT_NE(output.find("server"), std::string::npos);
    EXPECT_NE(output.find("[shared] hello from server"), std::string::npos);
}
