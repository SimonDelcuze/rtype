#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "lobby/RoomConfig.hpp"

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
    std::size_t playerCount;
    std::size_t maxPlayers;
    RoomState state;
    std::uint16_t port;
    std::uint32_t ownerId{0};
    std::vector<std::uint32_t> adminIds;
    std::vector<std::uint32_t> bannedPlayerIds;
    std::vector<std::string> bannedIPs;
    std::string roomName{"New Room"};
    bool passwordProtected{false};
    std::string passwordHash;
    RoomVisibility visibility{RoomVisibility::Public};
    std::string inviteCode;
    RoomConfig config{RoomConfig::preset(RoomDifficulty::Hell)};
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

    void setRoomOwner(std::uint32_t roomId, std::uint32_t ownerId);

    void addRoomAdmin(std::uint32_t roomId, std::uint32_t playerId);

    void removeRoomAdmin(std::uint32_t roomId, std::uint32_t playerId);

    void addBannedPlayer(std::uint32_t roomId, std::uint32_t playerId, const std::string& ipAddress);

    void removeBannedPlayer(std::uint32_t roomId, std::uint32_t playerId);

    bool isPlayerBanned(std::uint32_t roomId, std::uint32_t playerId, const std::string& ipAddress) const;

    void setRoomName(std::uint32_t roomId, const std::string& name);

    void setRoomPassword(std::uint32_t roomId, const std::string& passwordHash);

    void setRoomVisibility(std::uint32_t roomId, RoomVisibility visibility);
    void setRoomConfig(std::uint32_t roomId, const RoomConfig& config);

    std::string generateAndSetInviteCode(std::uint32_t roomId);

    bool verifyRoomPassword(std::uint32_t roomId, const std::string& passwordHash) const;

    void addPlayerToRoom(std::uint32_t roomId, std::uint32_t playerId);
    void removePlayerFromRoom(std::uint32_t roomId, std::uint32_t playerId);
    std::vector<std::uint32_t> getRoomPlayers(std::uint32_t roomId) const;
    bool handlePlayerDisconnect(std::uint32_t roomId, std::uint32_t playerId);

  private:
    mutable std::mutex roomsMutex_;
    std::map<std::uint32_t, RoomInfo> rooms_;
    std::map<std::uint32_t, std::vector<std::uint32_t>> roomPlayers_;
    std::uint32_t nextPlayerId_{1};
};
