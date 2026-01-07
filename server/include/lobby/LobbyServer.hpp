#pragma once

#include "console/ServerConsole.hpp"
#include "core/Session.hpp"
#include "game/GameInstanceManager.hpp"
#include "lobby/LobbyManager.hpp"
#include "network/PacketHeader.hpp"
#include "network/UdpSocket.hpp"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

class LobbyServer
{
  public:
    LobbyServer(std::uint16_t lobbyPort, std::uint16_t gameBasePort, std::uint32_t maxInstances,
                std::atomic<bool>& runningFlag);
    ~LobbyServer();

    bool start();
    void run();
    void stop();

  private:
    void receiveThread();
    void cleanupThread();
    void handlePacket(const std::uint8_t* data, std::size_t size, const IpEndpoint& from);
    void handleLobbyListRooms(const PacketHeader& hdr, const IpEndpoint& from);
    void handleLobbyCreateRoom(const PacketHeader& hdr, const IpEndpoint& from);
    void handleLobbyJoinRoom(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                             const IpEndpoint& from);

    void sendPacket(const std::vector<std::uint8_t>& packet, const IpEndpoint& to);

    std::string endpointToKey(const IpEndpoint& ep) const;

    ServerStats aggregateStats();

    std::uint16_t lobbyPort_;
    std::uint16_t gameBasePort_;
    std::uint32_t maxInstances_;
    std::atomic<bool>* running_{nullptr};

    UdpSocket lobbySocket_;
    std::atomic<bool> receiveRunning_{false};
    std::thread receiveWorker_;
    std::thread cleanupWorker_;

    mutable std::mutex sessionsMutex_;
    std::unordered_map<std::string, ClientSession> lobbySessions_;

    GameInstanceManager instanceManager_;
    LobbyManager lobbyManager_;
    std::unique_ptr<ServerConsole> tui_;

    std::uint16_t nextSequence_{0};
};
