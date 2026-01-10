#pragma once

#include "auth/AuthService.hpp"
#include "auth/Database.hpp"
#include "auth/UserRepository.hpp"
#include "console/ServerConsole.hpp"
#include "core/Session.hpp"
#include "game/GameInstanceManager.hpp"
#include "lobby/LobbyManager.hpp"
#include "lobby/LobbyPackets.hpp"
#include "network/AuthPackets.hpp"
#include "network/ChatPacket.hpp"
#include "network/PacketHeader.hpp"
#include "network/ServerBroadcastPacket.hpp"
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

    void broadcast(const std::string& message);
    void notifyDisconnection(const std::string& reason);

  private:
    void receiveThread();
    void cleanupThread();
    void handlePacket(const std::uint8_t* data, std::size_t size, const IpEndpoint& from);
    void handleLobbyListRooms(const PacketHeader& hdr, const IpEndpoint& from);
    void handleLobbyCreateRoom(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                               const IpEndpoint& from);
    void handleLobbyJoinRoom(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                             const IpEndpoint& from);
    void handleRoomGetPlayers(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                              const IpEndpoint& from);
    void handleRoomForceStart(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                              const IpEndpoint& from);
    void handleRoomKickPlayer(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                              const IpEndpoint& from);
    void handleLobbyLeaveRoom(const PacketHeader& hdr, const IpEndpoint& from);

    void handleLoginRequest(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                            const IpEndpoint& from);
    void handleRegisterRequest(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                               const IpEndpoint& from);
    void handleChangePasswordRequest(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                     const IpEndpoint& from);
    void handleGetStatsRequest(const PacketHeader& hdr, const IpEndpoint& from);
    void handleChatPacket(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size, const IpEndpoint& from);

    void sendPacket(const std::vector<std::uint8_t>& packet, const IpEndpoint& to);
    void sendAuthRequired(const IpEndpoint& to);

    std::string endpointToKey(const IpEndpoint& ep) const;
    bool isAuthenticated(const IpEndpoint& from) const;

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

    std::shared_ptr<Database> database_;
    std::shared_ptr<UserRepository> userRepository_;
    std::shared_ptr<AuthService> authService_;

    std::atomic<std::uint32_t> nextPlayerId_{1};
    std::uint16_t nextSequence_{0};
};
