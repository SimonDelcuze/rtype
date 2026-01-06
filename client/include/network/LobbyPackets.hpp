#pragma once

#include <cstdint>
#include <optional>
#include <vector>

enum class RoomState : std::uint8_t
{
    Waiting   = 0,
    Countdown = 1,
    Playing   = 2,
    Finished  = 3
};

struct RoomInfo
{
    std::uint32_t roomId;
    std::uint16_t playerCount;
    std::uint16_t maxPlayers;
    std::uint16_t port;
    RoomState state;
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

std::vector<std::uint8_t> buildListRoomsPacket(std::uint16_t sequence);

std::vector<std::uint8_t> buildCreateRoomPacket(std::uint16_t sequence);

std::vector<std::uint8_t> buildJoinRoomPacket(std::uint32_t roomId, std::uint16_t sequence);

std::optional<RoomListResult> parseRoomListPacket(const std::uint8_t* data, std::size_t size);

std::optional<RoomCreatedResult> parseRoomCreatedPacket(const std::uint8_t* data, std::size_t size);

std::optional<JoinSuccessResult> parseJoinSuccessPacket(const std::uint8_t* data, std::size_t size);
