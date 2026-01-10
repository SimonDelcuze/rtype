#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class RoomState : std::uint8_t
{
    Waiting   = 0,
    Countdown = 1,
    Playing   = 2,
    Finished  = 3
};

enum class RoomVisibility : std::uint8_t
{
    Public      = 0,
    Unlisted    = 1,
    FriendsOnly = 2,
    InviteOnly  = 3
};

struct RoomInfo
{
    std::uint32_t roomId;
    std::uint16_t playerCount;
    std::uint16_t maxPlayers;
    std::uint16_t port;
    RoomState state;
    std::uint32_t ownerId{0};
    bool passwordProtected{false};
    RoomVisibility visibility{RoomVisibility::Public};
    std::string inviteCode;
    std::string roomName{"New Room"};
};

struct RoomListResult
{
    std::vector<RoomInfo> rooms;
};

struct RoomCreatedResult
{
    std::uint32_t roomId;
    std::uint16_t port;
};

struct JoinSuccessResult
{
    std::uint32_t roomId;
    std::uint16_t port;
};

struct PlayerInfo
{
    std::uint32_t playerId;
    char name[32];
    bool isHost;
};

std::vector<std::uint8_t> buildListRoomsPacket(std::uint16_t sequence);

std::vector<std::uint8_t> buildCreateRoomPacket(const std::string& roomName, const std::string& passwordHash,
                                                RoomVisibility visibility, std::uint16_t sequence);

std::vector<std::uint8_t> buildJoinRoomPacket(std::uint32_t roomId, std::uint16_t sequence);
std::vector<std::uint8_t> buildJoinRoomPacket(std::uint32_t roomId, const std::string& passwordHash,
                                              std::uint16_t sequence);

std::optional<RoomListResult> parseRoomListPacket(const std::uint8_t* data, std::size_t size);

std::optional<RoomCreatedResult> parseRoomCreatedPacket(const std::uint8_t* data, std::size_t size);

std::optional<JoinSuccessResult> parseJoinSuccessPacket(const std::uint8_t* data, std::size_t size);

std::vector<std::uint8_t> buildGetPlayersPacket(std::uint32_t roomId, std::uint16_t sequence);

std::optional<std::vector<PlayerInfo>> parsePlayerListPacket(const std::uint8_t* data, std::size_t size);
