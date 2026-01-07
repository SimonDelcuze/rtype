#include "lobby/LobbyServer.hpp"

#include "Logger.hpp"
#include "lobby/LobbyPackets.hpp"
#include "server/Session.hpp"

#include <array>
#include <chrono>
#include <thread>

LobbyServer::LobbyServer(std::uint16_t lobbyPort, std::uint16_t gameBasePort, std::uint32_t maxInstances,
                         std::atomic<bool>& runningFlag)
    : lobbyPort_(lobbyPort), gameBasePort_(gameBasePort), maxInstances_(maxInstances), running_(&runningFlag),
      instanceManager_(gameBasePort, maxInstances, runningFlag)
{
    Logger::instance().info("[LobbyServer] Initialized on port " + std::to_string(lobbyPort) + " with game base port " +
                            std::to_string(gameBasePort));
}

LobbyServer::~LobbyServer()
{
    stop();
}

bool LobbyServer::start()
{
    IpEndpoint bind{.addr = {0, 0, 0, 0}, .port = lobbyPort_};

    if (!lobbySocket_.open(bind)) {
        Logger::instance().error("[LobbyServer] Failed to open lobby socket on port " + std::to_string(lobbyPort_));
        return false;
    }

    lobbySocket_.setNonBlocking(true);

    Logger::instance().info("[LobbyServer] Lobby socket opened on port " + std::to_string(lobbyPort_));

    receiveRunning_ = true;
    receiveWorker_  = std::thread([this]() { receiveThread(); });
    cleanupWorker_  = std::thread([this]() { cleanupThread(); });

    Logger::instance().info("[LobbyServer] Started receive and cleanup threads");

    tui_ = std::make_unique<ServerConsole>(&instanceManager_);
    tui_->setShutdownCallback([this]() {
        Logger::instance().info("[LobbyServer] Shutdown requested via TUI");
        if (running_) {
            *running_ = false;
        }
    });

    return true;
}

void LobbyServer::run()
{
    Logger::instance().info("[LobbyServer] Running...");
    while (running_ && running_->load()) {
        if (tui_) {
            tui_->handleInput();

            ServerStats stats = aggregateStats();
            tui_->update(stats);
            tui_->render();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void LobbyServer::stop()
{
    Logger::instance().info("[LobbyServer] Stopping...");
    receiveRunning_ = false;

    if (receiveWorker_.joinable()) {
        receiveWorker_.join();
    }

    if (cleanupWorker_.joinable()) {
        cleanupWorker_.join();
    }

    tui_.reset();

    Logger::instance().info("[LobbyServer] Stopped");
}

ServerStats LobbyServer::aggregateStats()
{
    ServerStats stats;
    stats.bytesIn     = Logger::instance().getTotalBytesReceived();
    stats.bytesOut    = Logger::instance().getTotalBytesSent();
    stats.packetsIn   = Logger::instance().getTotalPacketsReceived();
    stats.packetsOut  = Logger::instance().getTotalPacketsSent();
    stats.packetsLost = Logger::instance().getTotalPacketsDropped();
    stats.roomCount   = instanceManager_.getInstanceCount();

    auto roomIds = instanceManager_.getAllRoomIds();
    std::size_t totalClients = 0;
    for (auto roomId : roomIds) {
        auto* instance = instanceManager_.getInstance(roomId);
        if (instance != nullptr) {
            totalClients += instance->getPlayerCount();
        }
    }
    stats.clientCount = totalClients;

    return stats;
}

void LobbyServer::receiveThread()
{
    Logger::instance().info("[LobbyServer] Receive thread started");

    std::array<std::uint8_t, 2048> buffer{};

    while (receiveRunning_) {
        IpEndpoint from{};
        auto result = lobbySocket_.recvFrom(buffer.data(), buffer.size(), from);

        if (!result.ok() || result.size == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        handlePacket(buffer.data(), result.size, from);
    }

    Logger::instance().info("[LobbyServer] Receive thread stopped");
}

void LobbyServer::cleanupThread()
{
    Logger::instance().info("[LobbyServer] Cleanup thread started");

    while (receiveRunning_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        instanceManager_.cleanupEmptyInstances();

        auto activeRoomIds = instanceManager_.getAllRoomIds();
        auto lobbyRooms     = lobbyManager_.listRooms();

        for (const auto& room : lobbyRooms) {
            bool stillExists = false;
            for (auto activeId : activeRoomIds) {
                if (activeId == room.roomId) {
                    stillExists = true;
                    break;
                }
            }
            if (!stillExists) {
                lobbyManager_.removeRoom(room.roomId);
                Logger::instance().info("[LobbyServer] Removed room " + std::to_string(room.roomId) + " from lobby list");
            }
        }

        for (std::uint32_t roomId : activeRoomIds) {
            auto* instance = instanceManager_.getInstance(roomId);
            if (instance != nullptr) {
                lobbyManager_.updateRoomPlayerCount(roomId, instance->getPlayerCount());

                if (instance->isGameStarted()) {
                    lobbyManager_.updateRoomState(roomId, RoomState::Playing);
                } else {
                    lobbyManager_.updateRoomState(roomId, RoomState::Waiting);
                }
            }
        }
    }

    Logger::instance().info("[LobbyServer] Cleanup thread stopped");
}

void LobbyServer::handlePacket(const std::uint8_t* data, std::size_t size, const IpEndpoint& from)
{
    auto hdr = PacketHeader::decode(data, size);
    if (!hdr.has_value()) {
        Logger::instance().warn("[LobbyServer] Invalid packet header from " + std::to_string(from.addr[0]) + "." +
                                std::to_string(from.addr[1]) + "." + std::to_string(from.addr[2]) + "." +
                                std::to_string(from.addr[3]) + ":" + std::to_string(from.port));
        return;
    }

    auto msgType = static_cast<MessageType>(hdr->messageType);

    switch (msgType) {
        case MessageType::LobbyListRooms:
            handleLobbyListRooms(*hdr, from);
            break;

        case MessageType::LobbyCreateRoom:
            handleLobbyCreateRoom(*hdr, from);
            break;

        case MessageType::LobbyJoinRoom:
            handleLobbyJoinRoom(*hdr, data, size, from);
            break;

        default:
            Logger::instance().warn("[LobbyServer] Unhandled message type: " +
                                    std::to_string(static_cast<std::uint8_t>(msgType)));
            break;
    }
}

void LobbyServer::handleLobbyListRooms(const PacketHeader& hdr, const IpEndpoint& from)
{
    Logger::instance().info("[LobbyServer] List rooms request from client");

    auto rooms  = lobbyManager_.listRooms();
    auto packet = buildRoomListPacket(rooms, hdr.sequenceId);

    sendPacket(packet, from);
}

void LobbyServer::handleLobbyCreateRoom(const PacketHeader& hdr, const IpEndpoint& from)
{
    Logger::instance().info("[LobbyServer] Create room request from client");

    auto roomId = instanceManager_.createInstance();
    if (!roomId.has_value()) {
        Logger::instance().warn("[LobbyServer] Failed to create room (max instances reached)");
        auto packet = buildJoinFailedPacket(hdr.sequenceId);
        sendPacket(packet, from);
        return;
    }

    auto* instance = instanceManager_.getInstance(*roomId);
    if (instance == nullptr) {
        Logger::instance().error("[LobbyServer] Instance created but not found!");
        auto packet = buildJoinFailedPacket(hdr.sequenceId);
        sendPacket(packet, from);
        return;
    }

    std::uint16_t port = instance->getPort();

    lobbyManager_.addRoom(*roomId, port, 4);

    Logger::instance().info("[LobbyServer] Created room " + std::to_string(*roomId) + " on port " +
                            std::to_string(port));

    auto packet = buildRoomCreatedPacket(*roomId, port, hdr.sequenceId);
    sendPacket(packet, from);
}

void LobbyServer::handleLobbyJoinRoom(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                      const IpEndpoint& from)
{
    if (size < PacketHeader::kSize + sizeof(std::uint32_t)) {
        Logger::instance().warn("[LobbyServer] Join room packet too small");
        auto packet = buildJoinFailedPacket(hdr.sequenceId);
        sendPacket(packet, from);
        return;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;
    std::uint32_t roomId        = (static_cast<std::uint32_t>(payload[0]) << 24) |
                           (static_cast<std::uint32_t>(payload[1]) << 16) |
                           (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);

    Logger::instance().info("[LobbyServer] Join room " + std::to_string(roomId) + " request from client");

    if (!instanceManager_.hasInstance(roomId)) {
        Logger::instance().warn("[LobbyServer] Room " + std::to_string(roomId) + " does not exist");
        auto packet = buildJoinFailedPacket(hdr.sequenceId);
        sendPacket(packet, from);
        return;
    }

    auto* instance = instanceManager_.getInstance(roomId);
    if (instance == nullptr) {
        Logger::instance().error("[LobbyServer] Instance exists but not accessible!");
        auto packet = buildJoinFailedPacket(hdr.sequenceId);
        sendPacket(packet, from);
        return;
    }

    std::uint16_t port = instance->getPort();

    std::lock_guard<std::mutex> lock(sessionsMutex_);
    std::string key  = endpointToKey(from);
    auto& session    = lobbySessions_[key];
    session.endpoint = from;
    session.roomId   = roomId;

    Logger::instance().info("[LobbyServer] Client joining room " + std::to_string(roomId) + " on port " +
                            std::to_string(port));

    auto packet = buildJoinSuccessPacket(roomId, port, hdr.sequenceId);
    sendPacket(packet, from);
}

void LobbyServer::sendPacket(const std::vector<std::uint8_t>& packet, const IpEndpoint& to)
{
    lobbySocket_.sendTo(packet.data(), packet.size(), to);
}

std::string LobbyServer::endpointToKey(const IpEndpoint& ep) const
{
    return std::to_string(ep.addr[0]) + "." + std::to_string(ep.addr[1]) + "." + std::to_string(ep.addr[2]) + "." +
           std::to_string(ep.addr[3]) + ":" + std::to_string(ep.port);
}
