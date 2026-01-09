#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "network/LobbyPackets.hpp"
#include "network/PacketHeader.hpp"
#include "network/UdpSocket.hpp"
#include "ui/NotificationData.hpp"

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

  private:
    std::vector<std::uint8_t> sendAndWaitForResponse(const std::vector<std::uint8_t>& packet,
                                                     MessageType expectedResponse,
                                                     std::chrono::milliseconds timeout = std::chrono::seconds(1));

    IpEndpoint lobbyEndpoint_;
    UdpSocket socket_;
    const std::atomic<bool>& runningFlag_;
    std::uint16_t nextSequence_{0};
    bool serverLost_{false};
    bool gameStarting_{false};
    std::uint8_t expectedPlayerCount_{0};
    bool inRoom_{false};
    bool wasKicked_{false};
};
