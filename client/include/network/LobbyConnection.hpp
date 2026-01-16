#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "network/AuthPackets.hpp"
#include "network/ChatPacket.hpp"
#include "network/LeaderboardPacket.hpp"
#include "network/LobbyPackets.hpp"
#include "network/PacketHeader.hpp"
#include "network/StatsPackets.hpp"
#include "network/UdpSocket.hpp"
#include "ui/NotificationData.hpp"
#include "ui/RoomDifficulty.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

class LobbyConnection
{
  public:
    struct RoomConfigUpdate
    {
        std::uint32_t roomId{0};
        RoomDifficulty mode{RoomDifficulty::Noob};
        float enemyMultiplier{1.0F};
        float playerSpeedMultiplier{1.0F};
        float scoreMultiplier{1.0F};
        std::uint8_t playerLives{3};
    };

    LobbyConnection(const IpEndpoint& lobbyEndpoint, const std::atomic<bool>& runningFlag);
    ~LobbyConnection();

    bool connect();
    void disconnect();

    std::optional<RoomListResult> requestRoomList();

    std::optional<RoomCreatedResult> createRoom();
    std::optional<RoomCreatedResult> createRoom(const std::string& roomName, const std::string& passwordHash,
                                                RoomVisibility visibility);

    std::optional<JoinSuccessResult> joinRoom(std::uint32_t roomId);
    std::optional<JoinSuccessResult> joinRoom(std::uint32_t roomId, const std::string& passwordHash);
    std::optional<std::vector<PlayerInfo>> requestPlayerList(std::uint32_t roomId);
    void notifyGameStarting(std::uint32_t roomId);
    void kickPlayer(std::uint32_t roomId, std::uint32_t playerId);
    void leaveRoom();
    void poll(ThreadSafeQueue<NotificationData>& broadcastQueue);
    bool ping();
    bool isServerLost() const
    {
        return serverLost_;
    }
    bool isGameStarting() const
    {
        return gameStarting_;
    }
    std::uint8_t getExpectedPlayerCount() const
    {
        return expectedPlayerCount_;
    }
    bool wasKicked() const
    {
        return wasKicked_;
    }

    void sendLogin(const std::string& username, const std::string& password);
    bool hasLoginResult() const;
    std::optional<LoginResponseData> popLoginResult();

    void sendRegister(const std::string& username, const std::string& password);
    bool hasRegisterResult() const;
    std::optional<RegisterResponseData> popRegisterResult();

    void sendRequestRoomList();
    bool hasRoomListResult() const;
    std::optional<RoomListResult> popRoomListResult();

    void sendCreateRoom(const std::string& roomName = "New Room", const std::string& password = "",
                        RoomVisibility visibility = RoomVisibility::Public);
    bool hasRoomCreatedResult() const;
    std::optional<RoomCreatedResult> popRoomCreatedResult();

    void sendJoinRoom(std::uint32_t roomId, const std::string& password = "");
    bool hasJoinRoomResult() const;
    std::optional<JoinSuccessResult> popJoinRoomResult();

    void sendRequestPlayerList(std::uint32_t roomId);
    bool hasPlayerListResult() const;
    std::optional<std::vector<PlayerInfo>> popPlayerListResult();

    void sendRequestStats();
    bool hasStatsResult() const;
    std::optional<struct GetStatsResponseData> popStatsResult();

    void sendRequestLeaderboard();
    bool hasLeaderboardResult() const;
    std::optional<LeaderboardResponseData> popLeaderboardResult();

    void sendNotifyGameStarting(std::uint32_t roomId);

    void sendKickPlayer(std::uint32_t roomId, std::uint32_t playerId);
    void sendRoomConfig(std::uint32_t roomId, RoomDifficulty mode, float enemyMult, float playerSpeedMult,
                        float scoreMult, std::uint8_t lives);
    void sendSetReady(std::uint32_t roomId, bool ready);

    void sendLeaveRoom();
    std::optional<LoginResponseData> login(const std::string& username, const std::string& password);
    std::optional<RegisterResponseData> registerUser(const std::string& username, const std::string& password);
    std::optional<ChangePasswordResponseData> changePassword(const std::string& oldPassword,
                                                             const std::string& newPassword, const std::string& token);
    std::optional<struct GetStatsResponseData> getStats();

    void sendChatMessage(std::uint32_t roomId, const std::string& message);
    bool hasNewChatMessages() const;
    std::vector<ChatPacket> popChatMessages();

    bool hasRoomConfigUpdate() const
    {
        return pendingRoomConfig_.has_value();
    }
    std::optional<RoomConfigUpdate> popRoomConfigUpdate()
    {
        auto val = pendingRoomConfig_;
        pendingRoomConfig_.reset();
        return val;
    }
    std::uint8_t getRoomCountdown() const
    {
        return currentRoomCountdown_;
    }

  private:
    std::vector<std::uint8_t> sendAndWaitForResponse(const std::vector<std::uint8_t>& packet,
                                                     MessageType expectedResponse,
                                                     std::chrono::milliseconds timeout = std::chrono::seconds(1));

    void handleIncomingPacket(MessageType type, const std::uint8_t* data, std::size_t size, const IpEndpoint& from,
                              ThreadSafeQueue<NotificationData>* broadcastQueue = nullptr);

    IpEndpoint lobbyEndpoint_;
    UdpSocket socket_;
    const std::atomic<bool>& runningFlag_;
    std::uint16_t nextSequence_{0};
    bool serverLost_{false};
    bool gameStarting_{false};
    std::uint8_t expectedPlayerCount_{0};
    bool inRoom_{false};
    bool wasKicked_{false};

    std::optional<LoginResponseData> pendingLoginResult_;
    std::optional<RegisterResponseData> pendingRegisterResult_;
    std::optional<RoomListResult> pendingRoomListResult_;
    std::optional<RoomCreatedResult> pendingRoomCreatedResult_;
    std::optional<JoinSuccessResult> pendingJoinRoomResult_;
    std::optional<std::vector<PlayerInfo>> pendingPlayerListResult_;
    std::optional<LeaderboardResponseData> pendingLeaderboardResult_;
    std::optional<struct GetStatsResponseData> pendingStatsResult_;
    ThreadSafeQueue<ChatPacket> chatMessages_;
    std::optional<RoomConfigUpdate> pendingRoomConfig_;
    std::uint8_t currentRoomCountdown_{0};
};
