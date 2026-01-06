#pragma once

#include <cstdint>
#include <map>
#include <mutex>
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

struct RoomInfo
{
    std::uint32_t roomId;
    std::size_t playerCount;
    std::size_t maxPlayers;
    RoomState state;
    std::uint16_t port;
};

class LobbyManager
{
  public:
    LobbyManager();

    void addRoom(std::uint32_t roomId, std::uint16_t port, std::size_t maxPlayers = 4);

    void removeRoom(std::uint32_t roomId);

    void updateRoomState(std::uint32_t roomId, RoomState state);

    void updateRoomPlayerCount(std::uint32_t roomId, std::size_t playerCount);

    std::optional<RoomInfo> getRoomInfo(std::uint32_t roomId) const;

    std::vector<RoomInfo> listRooms() const;

    bool roomExists(std::uint32_t roomId) const;

  private:
    mutable std::mutex roomsMutex_;
    std::map<std::uint32_t, RoomInfo> rooms_;
};
