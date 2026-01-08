#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "network/LobbyPackets.hpp"
#include "network/PacketHeader.hpp"
#include "network/UdpSocket.hpp"

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
    void poll(ThreadSafeQueue<std::string>& broadcastQueue);

  private:
    std::vector<std::uint8_t> sendAndWaitForResponse(const std::vector<std::uint8_t>& packet,
                                                     MessageType expectedResponse,
                                                     std::chrono::milliseconds timeout = std::chrono::seconds(1));

    IpEndpoint lobbyEndpoint_;
    UdpSocket socket_;
    const std::atomic<bool>& runningFlag_;
    std::uint16_t nextSequence_{0};
};
