#pragma once

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
    LobbyConnection(const IpEndpoint& lobbyEndpoint);
    ~LobbyConnection();

    bool connect();
    void disconnect();

    std::optional<RoomListResult> requestRoomList();

    std::optional<RoomCreatedResult> createRoom();

    std::optional<JoinSuccessResult> joinRoom(std::uint32_t roomId);

  private:
    std::vector<std::uint8_t> sendAndWaitForResponse(const std::vector<std::uint8_t>& packet,
                                                     MessageType expectedResponse,
                                                     std::chrono::milliseconds timeout = std::chrono::seconds(5));

    IpEndpoint lobbyEndpoint_;
    UdpSocket socket_;
    std::uint16_t nextSequence_{0};
};
