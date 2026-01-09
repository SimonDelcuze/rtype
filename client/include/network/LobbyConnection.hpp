#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "network/AuthPackets.hpp"
#include "network/LobbyPackets.hpp"
#include "network/PacketHeader.hpp"
#include "network/StatsPackets.hpp"
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

    std::optional<JoinSuccessResult> joinRoom(std::uint32_t roomId);
    void poll(ThreadSafeQueue<NotificationData>& broadcastQueue);
    bool ping();
    bool isServerLost() const
    {
        return serverLost_;
    }

    std::optional<LoginResponseData> login(const std::string& username, const std::string& password);
    std::optional<RegisterResponseData> registerUser(const std::string& username, const std::string& password);
    std::optional<ChangePasswordResponseData> changePassword(const std::string& oldPassword,
                                                             const std::string& newPassword, const std::string& token);

    std::optional<struct GetStatsResponseData> getStats();

  private:
    std::vector<std::uint8_t> sendAndWaitForResponse(const std::vector<std::uint8_t>& packet,
                                                     MessageType expectedResponse,
                                                     std::chrono::milliseconds timeout = std::chrono::seconds(1));

    IpEndpoint lobbyEndpoint_;
    UdpSocket socket_;
    const std::atomic<bool>& runningFlag_;
    std::uint16_t nextSequence_{0};
    bool serverLost_{false};
};
