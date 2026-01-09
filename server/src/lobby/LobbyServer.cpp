#include "lobby/LobbyServer.hpp"

#include "Logger.hpp"
#include "core/Session.hpp"
#include "lobby/LobbyPackets.hpp"
#include "network/AuthPackets.hpp"
#include "network/ServerBroadcastPacket.hpp"
#include "network/StatsPackets.hpp"
#include "network/ServerDisconnectPacket.hpp"

#include <array>
#include <chrono>
#include <fstream>
#include <thread>

LobbyServer::LobbyServer(std::uint16_t lobbyPort, std::uint16_t gameBasePort, std::uint32_t maxInstances,
                         std::atomic<bool>& runningFlag)
    : lobbyPort_(lobbyPort), gameBasePort_(gameBasePort), maxInstances_(maxInstances), running_(&runningFlag),
      instanceManager_(gameBasePort, maxInstances, runningFlag)
{
    Logger::instance().info("[LobbyServer] Initialized on port " + std::to_string(lobbyPort) + " with game base port " +
                            std::to_string(gameBasePort));

    database_ = std::make_shared<Database>();
    if (!database_->initialize("data/rtype.db")) {
        Logger::instance().error("[LobbyServer] Failed to initialize database");
        return;
    }

    std::ifstream schemaFile("server/src/auth/migrations/001_initial_schema.sql");
    if (schemaFile.is_open()) {
        std::string schema((std::istreambuf_iterator<char>(schemaFile)), std::istreambuf_iterator<char>());
        if (!database_->executeScript(schema)) {
            Logger::instance().error("[LobbyServer] Failed to execute database schema");
        }
        schemaFile.close();
    }

    userRepository_ = std::make_shared<UserRepository>(database_);
    authService_    = std::make_shared<AuthService>("rtype-jwt-secret-key-change-in-production");

    Logger::instance().info("[LobbyServer] Authentication system initialized");
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

    tui_ = std::make_unique<ServerConsole>(&instanceManager_, &lobbyManager_);
    tui_->setBroadcastCallback([this](const std::string& msg) { broadcast(msg); });
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

    notifyDisconnection("Server disconnected");
    instanceManager_.stopAll("Server disconnected");

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

void LobbyServer::broadcast(const std::string& message)
{
    Logger::instance().info("[LobbyServer] Broadcast: " + message);

    auto pkt   = ServerBroadcastPacket::create(message);
    auto bytes = pkt.encode();
    std::vector<std::uint8_t> vec(bytes.begin(), bytes.end());

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        for (const auto& [key, session] : lobbySessions_) {
            sendPacket(vec, session.endpoint);
        }
    }

    instanceManager_.broadcast(message);
}

void LobbyServer::notifyDisconnection(const std::string& reason)
{
    Logger::instance().info("[LobbyServer] Notify disconnect: " + reason);

    auto pkt   = ServerDisconnectPacket::create(reason);
    auto bytes = pkt.encode();
    std::vector<std::uint8_t> vec(bytes.begin(), bytes.end());

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        for (const auto& [key, session] : lobbySessions_) {
            sendPacket(vec, session.endpoint);
        }
    }
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

    auto roomIds             = instanceManager_.getAllRoomIds();
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
        auto lobbyRooms    = lobbyManager_.listRooms();

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
                Logger::instance().info("[LobbyServer] Removed room " + std::to_string(room.roomId) +
                                        " from lobby list");
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
        case MessageType::AuthLoginRequest:
            handleLoginRequest(*hdr, data, size, from);
            break;

        case MessageType::AuthRegisterRequest:
            handleRegisterRequest(*hdr, data, size, from);
            break;

        case MessageType::AuthChangePasswordRequest:
            handleChangePasswordRequest(*hdr, data, size, from);
            break;

        case MessageType::AuthGetStatsRequest:
            handleGetStatsRequest(*hdr, from);
            break;

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

bool LobbyServer::isAuthenticated(const IpEndpoint& from) const
{
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    std::string key = endpointToKey(from);
    auto it         = lobbySessions_.find(key);
    return it != lobbySessions_.end() && it->second.authenticated;
}

void LobbyServer::sendAuthRequired(const IpEndpoint& to)
{
    auto packet = buildAuthRequiredPacket(nextSequence_++);
    sendPacket(packet, to);
}

void LobbyServer::handleLoginRequest(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                     const IpEndpoint& from)
{
    Logger::instance().info("[LobbyServer] Login request from client");

    auto loginData = parseLoginRequestPacket(data, size);
    if (!loginData.has_value()) {
        Logger::instance().warn("[LobbyServer] Invalid login request packet");
        auto response = buildLoginResponsePacket(false, 0, "", AuthErrorCode::ServerError, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    auto user = userRepository_->getUserByUsername(loginData->username);
    if (!user.has_value()) {
        Logger::instance().warn("[LobbyServer] Login failed: user not found - " + loginData->username);
        auto response = buildLoginResponsePacket(false, 0, "", AuthErrorCode::InvalidCredentials, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    if (!authService_->verifyPassword(loginData->password, user->passwordHash)) {
        Logger::instance().warn("[LobbyServer] Login failed: invalid password for " + loginData->username);
        auto response = buildLoginResponsePacket(false, 0, "", AuthErrorCode::InvalidCredentials, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    std::string token = authService_->generateJWT(user->id, user->username);
    userRepository_->updateLastLogin(user->id);

    std::lock_guard<std::mutex> lock(sessionsMutex_);
    std::string key       = endpointToKey(from);
    auto& session         = lobbySessions_[key];
    session.authenticated = true;
    session.userId        = user->id;
    session.username      = user->username;
    session.jwtToken      = token;
    session.endpoint      = from;

    Logger::instance().info("[LobbyServer] User " + user->username + " logged in successfully");

    auto response = buildLoginResponsePacket(true, user->id, token, AuthErrorCode::Success, hdr.sequenceId);
    sendPacket(response, from);
}

void LobbyServer::handleRegisterRequest(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                        const IpEndpoint& from)
{
    Logger::instance().info("[LobbyServer] Register request from client");

    auto registerData = parseRegisterRequestPacket(data, size);
    if (!registerData.has_value()) {
        Logger::instance().warn("[LobbyServer] Invalid register request packet");
        auto response = buildRegisterResponsePacket(false, 0, AuthErrorCode::ServerError, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    if (registerData->username.length() < 3 || registerData->username.length() > 32) {
        Logger::instance().warn("[LobbyServer] Register failed: invalid username length");
        auto response = buildRegisterResponsePacket(false, 0, AuthErrorCode::ServerError, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    if (registerData->password.length() < 8 || registerData->password.length() > 64) {
        Logger::instance().warn("[LobbyServer] Register failed: password too weak");
        auto response = buildRegisterResponsePacket(false, 0, AuthErrorCode::WeakPassword, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    auto existingUser = userRepository_->getUserByUsername(registerData->username);
    if (existingUser.has_value()) {
        Logger::instance().warn("[LobbyServer] Register failed: username already taken - " + registerData->username);
        auto response = buildRegisterResponsePacket(false, 0, AuthErrorCode::UsernameTaken, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    std::string passwordHash = authService_->hashPassword(registerData->password);
    auto userId              = userRepository_->createUser(registerData->username, passwordHash);

    if (!userId.has_value()) {
        Logger::instance().error("[LobbyServer] Register failed: database error");
        auto response = buildRegisterResponsePacket(false, 0, AuthErrorCode::ServerError, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    Logger::instance().info("[LobbyServer] User " + registerData->username + " registered successfully");

    auto response = buildRegisterResponsePacket(true, userId.value(), AuthErrorCode::Success, hdr.sequenceId);
    sendPacket(response, from);
}

void LobbyServer::handleChangePasswordRequest(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                              const IpEndpoint& from)
{
    Logger::instance().info("[LobbyServer] Change password request from client");

    if (!isAuthenticated(from)) {
        Logger::instance().warn("[LobbyServer] Change password failed: not authenticated");
        sendAuthRequired(from);
        return;
    }

    auto changeData = parseChangePasswordRequestPacket(data, size);
    if (!changeData.has_value()) {
        Logger::instance().warn("[LobbyServer] Invalid change password request packet");
        auto response = buildChangePasswordResponsePacket(false, AuthErrorCode::ServerError, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    auto payload = authService_->validateJWT(changeData->token);
    if (!payload.has_value() || !payload->isValid()) {
        Logger::instance().warn("[LobbyServer] Change password failed: invalid token");
        auto response = buildChangePasswordResponsePacket(false, AuthErrorCode::InvalidToken, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    auto user = userRepository_->getUserById(payload->userId);
    if (!user.has_value()) {
        Logger::instance().warn("[LobbyServer] Change password failed: user not found");
        auto response = buildChangePasswordResponsePacket(false, AuthErrorCode::ServerError, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    if (!authService_->verifyPassword(changeData->oldPassword, user->passwordHash)) {
        Logger::instance().warn("[LobbyServer] Change password failed: invalid old password");
        auto response = buildChangePasswordResponsePacket(false, AuthErrorCode::InvalidCredentials, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    if (changeData->newPassword.length() < 8 || changeData->newPassword.length() > 64) {
        Logger::instance().warn("[LobbyServer] Change password failed: new password too weak");
        auto response = buildChangePasswordResponsePacket(false, AuthErrorCode::WeakPassword, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    std::string newPasswordHash = authService_->hashPassword(changeData->newPassword);
    if (!userRepository_->updatePassword(payload->userId, newPasswordHash)) {
        Logger::instance().error("[LobbyServer] Change password failed: database error");
        auto response = buildChangePasswordResponsePacket(false, AuthErrorCode::ServerError, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    Logger::instance().info("[LobbyServer] Password changed successfully for user " + user->username);

    auto response = buildChangePasswordResponsePacket(true, AuthErrorCode::Success, hdr.sequenceId);
    sendPacket(response, from);
}

void LobbyServer::handleGetStatsRequest(const PacketHeader& hdr, const IpEndpoint& from)
{
    Logger::instance().info("[LobbyServer] Get stats request from client");

    if (!isAuthenticated(from)) {
        Logger::instance().warn("[LobbyServer] Get stats failed: not authenticated");
        sendAuthRequired(from);
        return;
    }

    std::string key = endpointToKey(from);
    ClientSession session;
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        auto it = lobbySessions_.find(key);
        if (it == lobbySessions_.end() || !it->second.authenticated || !it->second.userId.has_value()) {
            Logger::instance().warn("[LobbyServer] Get stats failed: session not found or not authenticated");
            sendAuthRequired(from);
            return;
        }
        session = it->second;
    }

    auto stats = userRepository_->getUserStats(session.userId.value());
    if (!stats.has_value()) {
        Logger::instance().warn("[LobbyServer] Get stats failed: stats not found for userId " +
                                std::to_string(session.userId.value()));

        GetStatsResponseData emptyStats;
        emptyStats.userId      = session.userId.value();
        emptyStats.gamesPlayed = 0;
        emptyStats.wins        = 0;
        emptyStats.losses      = 0;
        emptyStats.totalScore  = 0;

        auto response = buildGetStatsResponsePacket(emptyStats, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    GetStatsResponseData responseData;
    responseData.userId      = stats->userId;
    responseData.gamesPlayed = stats->gamesPlayed;
    responseData.wins        = stats->wins;
    responseData.losses      = stats->losses;
    responseData.totalScore  = stats->totalScore;

    Logger::instance().info("[LobbyServer] Sending stats for user " + session.username +
                            ": games=" + std::to_string(stats->gamesPlayed) + ", wins=" + std::to_string(stats->wins));

    auto response = buildGetStatsResponsePacket(responseData, hdr.sequenceId);
    sendPacket(response, from);
}
