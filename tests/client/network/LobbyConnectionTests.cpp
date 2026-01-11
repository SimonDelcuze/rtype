#include "network/LobbyConnection.hpp"

#include <atomic>
#include <gtest/gtest.h>

class LobbyConnectionTest : public ::testing::Test
{
  protected:
    std::atomic<bool> running{true};
    IpEndpoint endpoint = IpEndpoint::v4(127, 0, 0, 1, 1234);
};

TEST_F(LobbyConnectionTest, InitialState)
{
    LobbyConnection conn(endpoint, running);

    EXPECT_FALSE(conn.isServerLost());
    EXPECT_FALSE(conn.isGameStarting());
    EXPECT_FALSE(conn.wasKicked());
    EXPECT_FALSE(conn.hasLoginResult());
    EXPECT_FALSE(conn.hasRegisterResult());
    EXPECT_FALSE(conn.hasRoomListResult());
}

TEST_F(LobbyConnectionTest, PopLoginResultEmpty)
{
    LobbyConnection conn(endpoint, running);
    auto result = conn.popLoginResult();
    EXPECT_FALSE(result.has_value());
}
