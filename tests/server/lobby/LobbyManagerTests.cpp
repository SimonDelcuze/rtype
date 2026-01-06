#include "lobby/LobbyManager.hpp"

#include <gtest/gtest.h>

class LobbyManagerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        lobbyManager = std::make_unique<LobbyManager>();
    }

    std::unique_ptr<LobbyManager> lobbyManager;
};

TEST_F(LobbyManagerTest, InitiallyEmpty)
{
    auto rooms = lobbyManager->listRooms();
    EXPECT_TRUE(rooms.empty());
}

TEST_F(LobbyManagerTest, AddRoom)
{
    lobbyManager->addRoom(1, 50100, 4);

    EXPECT_TRUE(lobbyManager->roomExists(1));
    auto rooms = lobbyManager->listRooms();
    ASSERT_EQ(rooms.size(), 1);
    EXPECT_EQ(rooms[0].roomId, 1);
    EXPECT_EQ(rooms[0].port, 50100);
    EXPECT_EQ(rooms[0].maxPlayers, 4);
    EXPECT_EQ(rooms[0].playerCount, 0);
    EXPECT_EQ(rooms[0].state, RoomState::Waiting);
}

TEST_F(LobbyManagerTest, AddMultipleRooms)
{
    lobbyManager->addRoom(1, 50100, 4);
    lobbyManager->addRoom(2, 50101, 4);
    lobbyManager->addRoom(3, 50102, 2);

    auto rooms = lobbyManager->listRooms();
    ASSERT_EQ(rooms.size(), 3);

    EXPECT_TRUE(lobbyManager->roomExists(1));
    EXPECT_TRUE(lobbyManager->roomExists(2));
    EXPECT_TRUE(lobbyManager->roomExists(3));
    EXPECT_FALSE(lobbyManager->roomExists(4));
}

TEST_F(LobbyManagerTest, RemoveRoom)
{
    lobbyManager->addRoom(1, 50100, 4);
    lobbyManager->addRoom(2, 50101, 4);

    lobbyManager->removeRoom(1);

    EXPECT_FALSE(lobbyManager->roomExists(1));
    EXPECT_TRUE(lobbyManager->roomExists(2));

    auto rooms = lobbyManager->listRooms();
    ASSERT_EQ(rooms.size(), 1);
    EXPECT_EQ(rooms[0].roomId, 2);
}

TEST_F(LobbyManagerTest, RemoveNonExistentRoom)
{
    lobbyManager->removeRoom(999);

    auto rooms = lobbyManager->listRooms();
    EXPECT_TRUE(rooms.empty());
}

TEST_F(LobbyManagerTest, GetRoomInfo)
{
    lobbyManager->addRoom(1, 50100, 4);

    auto info = lobbyManager->getRoomInfo(1);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->roomId, 1);
    EXPECT_EQ(info->port, 50100);
    EXPECT_EQ(info->maxPlayers, 4);
    EXPECT_EQ(info->playerCount, 0);
}

TEST_F(LobbyManagerTest, GetNonExistentRoomInfo)
{
    auto info = lobbyManager->getRoomInfo(999);
    EXPECT_FALSE(info.has_value());
}

TEST_F(LobbyManagerTest, UpdateRoomPlayerCount)
{
    lobbyManager->addRoom(1, 50100, 4);
    lobbyManager->updateRoomPlayerCount(1, 2);

    auto info = lobbyManager->getRoomInfo(1);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->playerCount, 2);
}

TEST_F(LobbyManagerTest, UpdateRoomState)
{
    lobbyManager->addRoom(1, 50100, 4);
    lobbyManager->updateRoomState(1, RoomState::Playing);

    auto info = lobbyManager->getRoomInfo(1);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->state, RoomState::Playing);
}

TEST_F(LobbyManagerTest, UpdateNonExistentRoom)
{
    lobbyManager->updateRoomPlayerCount(999, 5);
    lobbyManager->updateRoomState(999, RoomState::Playing);

    EXPECT_FALSE(lobbyManager->roomExists(999));
}

TEST_F(LobbyManagerTest, RoomStateProgression)
{
    lobbyManager->addRoom(1, 50100, 4);

    auto checkState = [&](RoomState expected) {
        auto info = lobbyManager->getRoomInfo(1);
        ASSERT_TRUE(info.has_value());
        EXPECT_EQ(info->state, expected);
    };

    checkState(RoomState::Waiting);

    lobbyManager->updateRoomState(1, RoomState::Countdown);
    checkState(RoomState::Countdown);

    lobbyManager->updateRoomState(1, RoomState::Playing);
    checkState(RoomState::Playing);

    lobbyManager->updateRoomState(1, RoomState::Finished);
    checkState(RoomState::Finished);
}
