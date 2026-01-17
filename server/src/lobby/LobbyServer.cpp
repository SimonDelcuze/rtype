#include "lobby/LobbyServer.hpp"

#include "Logger.hpp"
#include "core/Session.hpp"
#include "lobby/LobbyPackets.hpp"
#include "lobby/PasswordUtils.hpp"
#include "lobby/RoomConfig.hpp"
#include "network/AuthPackets.hpp"
#include "network/ChatPacket.hpp"
#include "network/LeaderboardPacket.hpp"
#include "network/RoomType.hpp"
#include "network/ServerBroadcastPacket.hpp"
#include "network/ServerDisconnectPacket.hpp"
#include "network/StatsPackets.hpp"
#include "utils/StringSanity.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <thread>

namespace
{
    std::vector<std::uint8_t> buildRoomConfigPacket(std::uint32_t roomId, const RoomConfig& cfg, std::uint16_t seq)
    {
        auto encodePercent = [](float f) {
            int v = static_cast<int>(std::round(f * 100.0F));
            v     = std::clamp(v, 0, 1000);
            return static_cast<std::uint16_t>(v);
        };

        constexpr std::size_t payloadSize = sizeof(std::uint32_t) + 1 + sizeof(std::uint16_t) * 3 + 1;

        PacketHeader hdr{};
        hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        hdr.messageType = static_cast<std::uint8_t>(MessageType::RoomSetConfig);
        hdr.sequenceId  = seq;
        hdr.payloadSize = static_cast<std::uint16_t>(payloadSize);

        std::vector<std::uint8_t> pkt;
        pkt.reserve(PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize);
        auto headerBytes = hdr.encode();
        pkt.insert(pkt.end(), headerBytes.begin(), headerBytes.end());

        pkt.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>(roomId & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>(cfg.mode));

        auto enemyEnc  = encodePercent(cfg.enemyStatMultiplier);
        auto playerEnc = encodePercent(cfg.playerSpeedMultiplier);
        auto scoreEnc  = encodePercent(cfg.scoreMultiplier);

        pkt.push_back(static_cast<std::uint8_t>((enemyEnc >> 8) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>(enemyEnc & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>((playerEnc >> 8) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>(playerEnc & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>((scoreEnc >> 8) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>(scoreEnc & 0xFF));

        pkt.push_back(cfg.playerLives);

        auto crc = PacketHeader::crc32(pkt.data(), pkt.size());
        pkt.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>(crc & 0xFF));
        return pkt;
    }
} // namespace

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

    database_->execute("ALTER TABLE user_stats ADD COLUMN elo INTEGER NOT NULL DEFAULT 1000");
    database_->execute("ALTER TABLE user_stats ADD COLUMN total_ranked_score INTEGER NOT NULL DEFAULT 0");

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

    instanceManager_.setGameEndCallback([this](std::uint32_t roomId, const std::vector<PlayerGameResult>& results,
                                               bool isWin) {
        if (results.empty())
            return;

        long totalScoreInRoom = 0;
        for (const auto& res : results) {
            totalScoreInRoom += res.score;
        }

        Logger::instance().info("[LobbyServer] Game end callback for Room " + std::to_string(roomId) + ", " +
                                std::to_string(results.size()) + " players" + ", win=" +
                                std::string(isWin ? "Y" : "N") + ", totalScore=" + std::to_string(totalScoreInRoom));

        auto roomInfo = lobbyManager_.getRoomInfo(roomId);
        bool isRanked = roomInfo.has_value() && roomInfo->roomType == RoomType::Ranked;

        for (const auto& res : results) {
            auto userId       = res.userId;
            auto score        = res.score;
            auto currentStats = userRepository_->getUserStats(userId);

            uint32_t wins                  = 0;
            uint32_t losses                = 0;
            uint32_t games                 = 0;
            std::uint64_t totalScore       = 0;
            std::uint64_t totalRankedScore = 0;

            if (currentStats) {
                wins             = currentStats->wins;
                losses           = currentStats->losses;
                games            = currentStats->gamesPlayed;
                totalScore       = currentStats->totalScore;
                totalRankedScore = currentStats->totalRankedScore;
            }

            games++;
            if (isWin)
                wins++;
            else
                losses++;
            totalScore += score;
            if (isRanked) {
                totalRankedScore += score;
            }

            std::int32_t baseEloChange = isWin ? 15 : -15;
            float multiplier           = 1.0f;

            if (totalScoreInRoom > 0) {
                float contribution = static_cast<float>(score) / static_cast<float>(totalScoreInRoom);
                multiplier         = 0.5f + (contribution * static_cast<float>(results.size()) / 2.0f);
            } else if (results.size() > 1) {
                multiplier = 1.0f;
            }

            multiplier = std::clamp(multiplier, 0.3f, 2.0f);

            std::int32_t eloChange = 0;
            if (isRanked) {
                eloChange = static_cast<std::int32_t>(static_cast<float>(baseEloChange) * multiplier);

                if (isWin) {
                    eloChange = std::clamp(eloChange, 5, 25);
                } else {
                    eloChange = std::clamp(eloChange, -25, -5);
                }
            }

            std::int32_t newElo = (currentStats ? currentStats->elo : 1000) + eloChange;
            if (newElo < 0)
                newElo = 0;

            Logger::instance().info(
                "[LobbyServer] Updating stats for user " + std::to_string(userId) + ": score=" + std::to_string(score) +
                " (contrib=" + std::to_string(totalScoreInRoom > 0 ? (float) score / totalScoreInRoom : 0.0f) +
                "), ELO=" + std::to_string(newElo) + " (" + (eloChange >= 0 ? "+" : "") + std::to_string(eloChange) +
                ")");

            userRepository_->updateUserStats(userId, games, wins, losses, totalScore, totalRankedScore, newElo);

            {
                std::lock_guard<std::mutex> lock(sessionsMutex_);
                for (auto& [key, session] : lobbySessions_) {
                    if (session.userId.has_value() && session.userId.value() == userId) {
                        session.elo = newElo;
                        break;
                    }
                }
            }
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

    std::size_t lobbyClientCount = 0;
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        lobbyClientCount = lobbySessions_.size();
    }

    auto roomIds                                     = instanceManager_.getAllRoomIds();
    [[maybe_unused]] std::size_t gameInstancePlayers = 0;
    for (auto roomId : roomIds) {
        auto* instance = instanceManager_.getInstance(roomId);
        if (instance != nullptr) {
            gameInstancePlayers += instance->getPlayerCount();
        }
    }

    stats.clientCount = lobbyClientCount + gameInstancePlayers;

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

        Logger::instance().addPacketReceived();
        Logger::instance().addBytesReceived(result.size);

        handlePacket(buffer.data(), result.size, from);
    }

    Logger::instance().info("[LobbyServer] Receive thread stopped");
}

void LobbyServer::cleanupThread()
{
    Logger::instance().info("[LobbyServer] Cleanup thread started");

    while (receiveRunning_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        lobbyManager_.updateRankedCountdowns(0.1F);

        std::uint32_t roomToStart = 0;
        {
            auto rooms = lobbyManager_.listRooms();
            for (const auto& room : rooms) {
                if (room.roomType == RoomType::Ranked && room.playerCount >= 1) {
                    if (room.state == RoomState::Waiting && room.countdown == 0) {
                        roomToStart = room.roomId;
                        break;
                    }
                }
            }
        }

        if (roomToStart != 0) {
            Logger::instance().info("[LobbyServer] Ranked room " + std::to_string(roomToStart) +
                                    " timer expired, auto-starting.");
            PacketHeader dummyHdr{};
            dummyHdr.sequenceId = 0;
            std::vector<std::uint8_t> dummyData(PacketHeader::kSize + 4, 0);
            dummyData[PacketHeader::kSize + 0] = (roomToStart >> 24) & 0xFF;
            dummyData[PacketHeader::kSize + 1] = (roomToStart >> 16) & 0xFF;
            dummyData[PacketHeader::kSize + 2] = (roomToStart >> 8) & 0xFF;
            dummyData[PacketHeader::kSize + 3] = roomToStart & 0xFF;
            handleRoomForceStart(dummyHdr, dummyData.data(), dummyData.size(), IpEndpoint{});
        }

        {
            std::lock_guard<std::mutex> lock(sessionsMutex_);
            auto now = std::chrono::steady_clock::now();

            for (auto it = lobbySessions_.begin(); it != lobbySessions_.end();) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastActivity).count();
                if (elapsed > 30) {
                    bool isPlaying = false;
                    if (it->second.roomId != 0) {
                        auto room = lobbyManager_.getRoomInfo(it->second.roomId);
                        if (room && room->state == RoomState::Playing) {
                            isPlaying = true;
                        }
                    }

                    if (!isPlaying) {
                        Logger::instance().info("[LobbyServer] Removing inactive session: " + it->first);
                        it = lobbySessions_.erase(it);
                    } else {
                        ++it;
                    }
                } else {
                    ++it;
                }
            }
        }

        auto activeRoomIds = instanceManager_.getAllRoomIds();
        auto lobbyRooms    = lobbyManager_.listRooms();
        for (std::uint32_t roomId : activeRoomIds) {
            if (!lobbyManager_.roomExists(roomId)) {
                auto* instance = instanceManager_.getInstance(roomId);
                if (instance != nullptr) {
                    lobbyManager_.addRoom(roomId, instance->getPort(), 4, RoomType::Quickplay);
                    Logger::instance().warn("[LobbyServer] Re-registered missing room " + std::to_string(roomId) +
                                            " on port " + std::to_string(instance->getPort()));
                }
            }
        }

        for (std::uint32_t roomId : activeRoomIds) {
            auto* instance = instanceManager_.getInstance(roomId);
            if (instance != nullptr) {
                auto players = lobbyManager_.getRoomPlayers(roomId);
                lobbyManager_.updateRoomPlayerCount(roomId, players.size());

                if (instance->isGameStarted()) {
                    lobbyManager_.updateRoomState(roomId, RoomState::Playing);
                } else {
                    lobbyManager_.updateRoomState(roomId, RoomState::Waiting);
                }
            }
        }

        ensureRankedRoomExists();
    }

    Logger::instance().info("[LobbyServer] Cleanup thread stopped");
}

void LobbyServer::ensureRankedRoomExists()
{
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        if (lobbySessions_.empty()) {
            return;
        }
    }

    if (lobbyManager_.hasWaitingRoomOfType(RoomType::Ranked)) {
        return;
    }

    RoomConfig rankedCfg = RoomConfig::preset(RoomDifficulty::Nightmare);
    auto roomIdOpt       = instanceManager_.createInstance(rankedCfg);
    if (!roomIdOpt.has_value()) {
        Logger::instance().warn("[LobbyServer] Unable to auto-create ranked room (no capacity)");
        return;
    }

    auto* instance = instanceManager_.getInstance(*roomIdOpt);
    if (instance == nullptr) {
        Logger::instance().error("[LobbyServer] Auto-created ranked instance missing");
        return;
    }

    std::uint16_t port = instance->getPort();
    lobbyManager_.addRoom(*roomIdOpt, port, 4, RoomType::Ranked);
    lobbyManager_.setRoomConfig(*roomIdOpt, rankedCfg);
    lobbyManager_.setRoomName(*roomIdOpt, "Ranked");

    Logger::instance().info("[LobbyServer] Auto-created ranked room " + std::to_string(*roomIdOpt) + " on port " +
                            std::to_string(port));
}

void LobbyServer::handlePacket(const std::uint8_t* data, std::size_t size, const IpEndpoint& from)
{
    auto hdr = PacketHeader::decode(data, size);
    if (!hdr.has_value()) {
        Logger::instance().warn("[LobbyServer] Invalid packet header from " + endpointToKey(from));
        return;
    }
    auto msgType = static_cast<MessageType>(hdr->messageType);

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        std::string key      = endpointToKey(from);
        auto& session        = lobbySessions_[key];
        session.endpoint     = from;
        session.lastActivity = std::chrono::steady_clock::now();
    }

    switch (msgType) {
        case MessageType::ClientDisconnect: {
            std::lock_guard<std::mutex> lock(sessionsMutex_);
            std::string key = endpointToKey(from);
            lobbySessions_.erase(key);
            Logger::instance().info("[LobbyServer] Client disconnected (explicitly): " + key);
        } break;

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
            handleLobbyCreateRoom(*hdr, data, size, from);
            break;

        case MessageType::LobbyJoinRoom:
            handleLobbyJoinRoom(*hdr, data, size, from);
            break;

        case MessageType::RoomGetPlayers:
            handleRoomGetPlayers(*hdr, data, size, from);
            break;

        case MessageType::RoomForceStart:
            handleRoomForceStart(*hdr, data, size, from);
            break;

        case MessageType::RoomKickPlayer:
            handleRoomKickPlayer(*hdr, data, size, from);
            break;

        case MessageType::RoomSetConfig:
            handleRoomSetConfig(*hdr, data, size, from);
            break;

        case MessageType::RoomSetReady:
            handleRoomSetReady(*hdr, data, size, from);
            break;

        case MessageType::LobbyLeaveRoom:
            handleLobbyLeaveRoom(*hdr, from);
            break;

        case MessageType::Chat:
            handleChatPacket(*hdr, data, size, from);
            break;

        case MessageType::LeaderboardRequest:
            handleLeaderboardRequest(*hdr, from);
            break;

        default:
            Logger::instance().warn("[LobbyServer] Unhandled message type: " +
                                    std::to_string(static_cast<std::uint8_t>(msgType)));
            break;
    }
}

void LobbyServer::handleLobbyListRooms(const PacketHeader& hdr, const IpEndpoint& from)
{
    Logger::instance().info("[LobbyServer] List rooms request from client " + endpointToKey(from));

    for (auto roomId : instanceManager_.getAllRoomIds()) {
        if (!lobbyManager_.roomExists(roomId)) {
            if (auto* inst = instanceManager_.getInstance(roomId)) {
                lobbyManager_.addRoom(roomId, inst->getPort(), 4, RoomType::Quickplay);
                Logger::instance().warn("[LobbyServer] Re-registered missing room " + std::to_string(roomId) +
                                        " during list request");
            }
        }
    }

    auto rooms = lobbyManager_.listRooms();

    for (auto& room : rooms) {
        auto players              = lobbyManager_.getRoomPlayers(room.roomId);
        std::size_t nonSpectators = 0;
        for (auto playerId : players) {
            if (!lobbyManager_.isPlayerSpectator(room.roomId, playerId)) {
                nonSpectators++;
            }
        }
        room.playerCount = nonSpectators;
    }

    auto packet = buildRoomListPacket(rooms, hdr.sequenceId);

    Logger::instance().info("[LobbyServer] Sending room list (" + std::to_string(rooms.size()) + " rooms, " +
                            std::to_string(packet.size()) + " bytes) to " + endpointToKey(from));

    sendPacket(packet, from);
}

void LobbyServer::handleLobbyCreateRoom(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                        const IpEndpoint& from)
{
    Logger::instance().info("[LobbyServer] Create room request from client");

    std::string roomName      = "New Room";
    std::string passwordHash  = "";
    RoomVisibility visibility = RoomVisibility::Public;

    if (hdr.payloadSize > 0 && size >= PacketHeader::kSize + hdr.payloadSize) {
        const std::uint8_t* payload = data + PacketHeader::kSize;
        const std::uint8_t* ptr     = payload;
        const std::uint8_t* end     = payload + hdr.payloadSize;

        if (ptr < end) {
            visibility = static_cast<RoomVisibility>(*ptr);
            ptr += 1;
        }

        if (ptr + 2 <= end) {
            std::uint16_t nameLen = (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
            ptr += 2;

            if (ptr + nameLen <= end) {
                roomName = std::string(reinterpret_cast<const char*>(ptr), nameLen);
                ptr += nameLen;
            }
        }

        if (ptr + 2 <= end) {
            std::uint16_t passLen = (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
            ptr += 2;

            if (ptr + passLen <= end) {
                passwordHash = std::string(reinterpret_cast<const char*>(ptr), passLen);
                ptr += passLen;
            }
        }
    }

    RoomConfig roomConfig = RoomConfig::preset(RoomDifficulty::Hell);

    auto roomId = instanceManager_.createInstance(roomConfig);
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

    lobbyManager_.addRoom(*roomId, port, 4, RoomType::Quickplay);
    lobbyManager_.setRoomConfig(*roomId, roomConfig);

    lobbyManager_.setRoomName(*roomId, roomName);
    if (!passwordHash.empty()) {
        lobbyManager_.setRoomPassword(*roomId, PasswordUtils::hashPassword(passwordHash));
    }
    lobbyManager_.setRoomVisibility(*roomId, visibility);

    std::string inviteCode = lobbyManager_.generateAndSetInviteCode(*roomId);

    std::uint32_t playerId  = 0;
    std::string displayName = "";
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        std::string key  = endpointToKey(from);
        auto& session    = lobbySessions_[key];
        session.endpoint = from;
        session.roomId   = *roomId;

        if (session.playerId == 0) {
            session.playerId = nextPlayerId_++;
        }
        playerId = session.playerId;
        if (!session.playerName.empty()) {
            displayName = session.playerName;
        } else if (!session.username.empty()) {
            displayName = session.username;
        }

        if (session.userId.has_value()) {
            auto stats = userRepository_->getUserStats(session.userId.value());
            if (stats.has_value()) {
                session.elo = stats->elo;
            }
        }
    }

    lobbyManager_.addPlayerToRoom(*roomId, playerId, displayName);
    lobbyManager_.setRoomOwner(*roomId, playerId);

    Logger::instance().info("[LobbyServer] Created room '" + roomName + "' (ID: " + std::to_string(*roomId) +
                            ") on port " + std::to_string(port) + " with invite code: " + inviteCode +
                            " | Owner: " + std::to_string(playerId));

    auto packet = buildRoomCreatedPacket(*roomId, port, hdr.sequenceId);
    sendPacket(packet, from);
    auto cfgPkt = buildRoomConfigPacket(*roomId, roomConfig, hdr.sequenceId);
    sendPacket(cfgPkt, from);
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

    std::string passwordHash = "";
    bool isSpectator         = false;
    if (hdr.payloadSize > sizeof(std::uint32_t)) {
        const std::uint8_t* ptr = payload + sizeof(std::uint32_t);
        const std::uint8_t* end = data + PacketHeader::kSize + hdr.payloadSize;

        if (ptr + 2 <= end) {
            std::uint16_t passLen = (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
            ptr += 2;

            if (ptr + passLen <= end) {
                passwordHash = std::string(reinterpret_cast<const char*>(ptr), passLen);
                ptr += passLen;

                if (ptr < end) {
                    isSpectator = (ptr[0] != 0);
                }
            }
        }
    }

    Logger::instance().info("[LobbyServer] Join room " + std::to_string(roomId) + " request from client");

    if (!instanceManager_.hasInstance(roomId)) {
        Logger::instance().warn("[LobbyServer] Room " + std::to_string(roomId) + " does not exist");
        auto packet = buildJoinFailedPacket(hdr.sequenceId);
        sendPacket(packet, from);
        return;
    }

    std::string clientIP = endpointToKey(from);
    if (lobbyManager_.isPlayerBanned(roomId, 0, clientIP)) {
        Logger::instance().warn("[LobbyServer] Banned client " + clientIP + " attempted to join room " +
                                std::to_string(roomId));
        auto packet = buildJoinFailedPacket(hdr.sequenceId);
        sendPacket(packet, from);
        return;
    }

    auto roomInfoOpt = lobbyManager_.getRoomInfo(roomId);
    if (roomInfoOpt.has_value() && roomInfoOpt->passwordProtected) {
        if (passwordHash.empty() || PasswordUtils::hashPassword(passwordHash) != roomInfoOpt->passwordHash) {
            Logger::instance().warn("[LobbyServer] Client provided incorrect password for room " +
                                    std::to_string(roomId));
            auto packet = buildJoinFailedPacket(hdr.sequenceId);
            sendPacket(packet, from);
            return;
        }
    }

    auto* instance = instanceManager_.getInstance(roomId);
    if (instance == nullptr) {
        Logger::instance().error("[LobbyServer] Instance exists but not accessible!");
        auto packet = buildJoinFailedPacket(hdr.sequenceId);
        sendPacket(packet, from);
        return;
    }

    std::uint16_t port = instance->getPort();

    std::uint32_t playerId  = 0;
    std::string displayName = "";
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        std::string key  = endpointToKey(from);
        auto& session    = lobbySessions_[key];
        session.endpoint = from;
        session.roomId   = roomId;

        if (session.playerId == 0) {
            session.playerId = nextPlayerId_++;
        }
        playerId = session.playerId;
        if (!session.playerName.empty()) {
            displayName = session.playerName;
        } else if (!session.username.empty()) {
            displayName = session.username;
        }

        if (session.userId.has_value()) {
            auto stats = userRepository_->getUserStats(session.userId.value());
            if (stats.has_value()) {
                session.elo = stats->elo;
            }
        }
    }

    lobbyManager_.addPlayerToRoom(roomId, playerId, displayName);

    if (isSpectator) {
        lobbyManager_.setPlayerSpectator(roomId, playerId, true);
    }

    if (lobbyManager_.getRoomPlayers(roomId).size() == 1) {
        lobbyManager_.setRoomOwner(roomId, playerId);
        Logger::instance().info("[LobbyServer] Player " + std::to_string(playerId) + " is now owner of room " +
                                std::to_string(roomId));
    }

    Logger::instance().info("[LobbyServer] Client joining room " + std::to_string(roomId) + " on port " +
                            std::to_string(port) + " as player " + std::to_string(playerId) +
                            (isSpectator ? " [SPECTATOR]" : ""));

    auto packet = buildJoinSuccessPacket(roomId, port, hdr.sequenceId);
    sendPacket(packet, from);
    if (auto roomInfoOpt = lobbyManager_.getRoomInfo(roomId); roomInfoOpt.has_value()) {
        auto cfgPacket = buildRoomConfigPacket(roomId, roomInfoOpt->config, hdr.sequenceId);
        sendPacket(cfgPacket, from);
    }
}

void LobbyServer::sendPacket(const std::vector<std::uint8_t>& packet, const IpEndpoint& to)
{
    auto res = lobbySocket_.sendTo(packet.data(), packet.size(), to);
    if (!res.ok() || res.size != packet.size()) {
        Logger::instance().warn("[LobbyServer] sendPacket failed to " + endpointToKey(to) +
                                " err=" + std::to_string(static_cast<int>(res.error)) +
                                " sent=" + std::to_string(res.size) + "/" + std::to_string(packet.size()));
    }

    Logger::instance().addPacketSent();
    Logger::instance().addBytesSent(packet.size());
}

void LobbyServer::handleRoomGetPlayers(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                       const IpEndpoint& from)
{
    if (size < PacketHeader::kSize + sizeof(std::uint32_t)) {
        Logger::instance().warn("[LobbyServer] Invalid RoomGetPlayers packet size");
        return;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;

    std::uint32_t roomId = (static_cast<std::uint32_t>(payload[0]) << 24) |
                           (static_cast<std::uint32_t>(payload[1]) << 16) |
                           (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);

    Logger::instance().info("[LobbyServer] Client requesting player list for room " + std::to_string(roomId));

    auto players             = lobbyManager_.getRoomPlayers(roomId);
    std::uint8_t playerCount = static_cast<std::uint8_t>(players.size());
    std::uint8_t countdown   = lobbyManager_.getRoomCountdown(roomId);

    std::uint16_t payloadSize =
        sizeof(std::uint32_t) + sizeof(std::uint8_t) + sizeof(std::uint8_t) + (playerCount * 47);

    PacketHeader respHdr{};
    respHdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    respHdr.messageType = static_cast<std::uint8_t>(MessageType::RoomPlayerList);
    respHdr.sequenceId  = hdr.sequenceId;
    respHdr.payloadSize = payloadSize;

    auto encoded = respHdr.encode();
    std::vector<std::uint8_t> packet(encoded.begin(), encoded.end());

    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    packet.push_back(countdown);
    packet.push_back(playerCount);

    auto roomInfoOpt      = lobbyManager_.getRoomInfo(roomId);
    std::uint32_t ownerId = roomInfoOpt.has_value() ? roomInfoOpt->ownerId : 0;

    for (std::uint32_t playerId : players) {
        packet.push_back(static_cast<std::uint8_t>((playerId >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((playerId >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((playerId >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(playerId & 0xFF));

        std::uint32_t userId = 0;
        {
            std::lock_guard<std::mutex> lock(sessionsMutex_);
            for (const auto& [key, session] : lobbySessions_) {
                if (session.playerId == playerId && session.userId.has_value()) {
                    userId = session.userId.value();
                    break;
                }
            }
        }
        packet.push_back(static_cast<std::uint8_t>((userId >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((userId >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((userId >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(userId & 0xFF));

        std::string name = "Player " + std::to_string(playerId);
        if (auto storedName = lobbyManager_.getPlayerName(roomId, playerId); storedName.has_value()) {
            name = *storedName;
        } else {
            std::lock_guard<std::mutex> lock(sessionsMutex_);
            for (const auto& [key, session] : lobbySessions_) {
                if (session.playerId == playerId) {
                    if (!session.username.empty()) {
                        name = session.username;
                        break;
                    }
                    if (!session.playerName.empty()) {
                        name = session.playerName;
                        break;
                    }
                }
            }
        }
        for (int i = 0; i < 32; ++i) {
            if (i < static_cast<int>(name.size())) {
                packet.push_back(static_cast<std::uint8_t>(name[i]));
            } else {
                packet.push_back(0);
            }
        }

        bool isHost = (playerId == ownerId);
        packet.push_back(isHost ? 1 : 0);

        bool isReady = lobbyManager_.isPlayerReady(roomId, playerId);
        packet.push_back(isReady ? 1 : 0);

        bool isSpectator = lobbyManager_.isPlayerSpectator(roomId, playerId);
        packet.push_back(isSpectator ? 1 : 0);

        std::int32_t playerElo = 1000;
        {
            std::lock_guard<std::mutex> lock(sessionsMutex_);
            for (const auto& [key, session] : lobbySessions_) {
                if (session.playerId == playerId) {
                    playerElo = session.elo;
                    break;
                }
            }
        }
        packet.push_back(static_cast<std::uint8_t>((playerElo >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((playerElo >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((playerElo >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(playerElo & 0xFF));
    }

    auto crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    Logger::instance().info("[LobbyServer] Sending player list for room " + std::to_string(roomId) + ": " +
                            std::to_string(playerCount) + " players");

    sendPacket(packet, from);
}

void LobbyServer::handleRoomForceStart(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                       const IpEndpoint& from)
{
    (void) from;

    if (size < PacketHeader::kSize + sizeof(std::uint32_t)) {
        Logger::instance().warn("[LobbyServer] Invalid RoomForceStart packet size");
        return;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;

    std::uint32_t roomId = (static_cast<std::uint32_t>(payload[0]) << 24) |
                           (static_cast<std::uint32_t>(payload[1]) << 16) |
                           (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);

    auto roomInfo = lobbyManager_.getRoomInfo(roomId);
    if (!roomInfo.has_value()) {
        Logger::instance().error("[LobbyServer] Room " + std::to_string(roomId) + " not found");
        return;
    }

    std::string senderKey        = endpointToKey(from);
    std::uint32_t senderPlayerId = 0;
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        if (lobbySessions_.contains(senderKey)) {
            senderPlayerId = lobbySessions_[senderKey].playerId;
        }
    }

    auto players = lobbyManager_.getRoomPlayers(roomId);

    bool isRankedAndAllReady =
        (roomInfo->roomType == RoomType::Ranked && players.size() >= 1 && lobbyManager_.isRoomAllReady(roomId));
    bool isExpiredRanked =
        (roomInfo->roomType == RoomType::Ranked && players.size() >= 1 && lobbyManager_.getRoomCountdown(roomId) == 0);

    if (senderPlayerId != roomInfo->ownerId && !isRankedAndAllReady && !isExpiredRanked) {
        Logger::instance().warn("[LobbyServer] Player " + std::to_string(senderPlayerId) +
                                " tried to force start room " + std::to_string(roomId) +
                                " but is not the owner (Owner: " + std::to_string(roomInfo->ownerId) + ")");
        return;
    }

    std::uint8_t playerCount = static_cast<std::uint8_t>(players.size());

    Logger::instance().info("[LobbyServer] Owner validated. Starting Room " + std::to_string(roomId) + " with " +
                            std::to_string(playerCount) + " players (IDs: " + std::to_string(senderPlayerId) + ")");

    std::uint16_t gamePort = roomInfo->port;

    PacketHeader respHdr{};
    respHdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    respHdr.messageType = static_cast<std::uint8_t>(MessageType::RoomGameStarting);
    respHdr.sequenceId  = hdr.sequenceId;
    respHdr.payloadSize = sizeof(std::uint32_t) + sizeof(std::uint8_t) + sizeof(std::uint16_t);

    auto encoded = respHdr.encode();
    std::vector<std::uint8_t> packet(encoded.begin(), encoded.end());

    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    packet.push_back(playerCount);

    packet.push_back(static_cast<std::uint8_t>((gamePort >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(gamePort & 0xFF));

    auto crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    auto* instance = instanceManager_.getInstance(roomId);
    if (instance) {
        ControlEvent ctrl;
        ctrl.header = hdr;
        ctrl.from   = IpEndpoint{.addr = {0, 0, 0, 0}, .port = 0};
        ctrl.data.resize(sizeof(std::uint32_t) + sizeof(std::uint8_t));
        ctrl.data[0] = static_cast<std::uint8_t>((roomId >> 24) & 0xFF);
        ctrl.data[1] = static_cast<std::uint8_t>((roomId >> 16) & 0xFF);
        ctrl.data[2] = static_cast<std::uint8_t>((roomId >> 8) & 0xFF);
        ctrl.data[3] = static_cast<std::uint8_t>(roomId & 0xFF);
        ctrl.data[4] = playerCount;

        instance->handleControlEvent(ctrl);
        Logger::instance().info("[LobbyServer] Sent authoritative force start for room " + std::to_string(roomId) +
                                " with " + std::to_string(playerCount) + " players");
    }

    std::lock_guard<std::mutex> lock(sessionsMutex_);
    int broadcastCount = 0;
    for (const auto& [key, session] : lobbySessions_) {
        if (session.roomId == roomId) {
            Logger::instance().info("[LobbyServer] Sending RoomGameStarting to player in room " +
                                    std::to_string(roomId) + " at " + key +
                                    " (PlayerId=" + std::to_string(session.playerId) + ")");
            sendPacket(packet, session.endpoint);
            broadcastCount++;
        }
    }
    Logger::instance().info("[LobbyServer] Broadcast complete: sent to " + std::to_string(broadcastCount) + " players");
}

void LobbyServer::handleLobbyLeaveRoom(const PacketHeader& hdr, const IpEndpoint& from)
{
    (void) hdr;

    std::string key        = endpointToKey(from);
    std::uint32_t roomId   = 0;
    std::uint32_t playerId = 0;

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        auto it = lobbySessions_.find(key);
        if (it == lobbySessions_.end()) {
            Logger::instance().warn("[LobbyServer] Leave room request from unknown session");
            return;
        }

        roomId   = it->second.roomId;
        playerId = it->second.playerId;

        it->second.roomId = 0;
    }

    if (roomId == 0) {
        Logger::instance().warn("[LobbyServer] Player " + std::to_string(playerId) + " not in any room");
        return;
    }

    Logger::instance().info("[LobbyServer] Player " + std::to_string(playerId) + " leaving room " +
                            std::to_string(roomId));

    bool roomDeleted = lobbyManager_.handlePlayerDisconnect(roomId, playerId);

    if (roomDeleted) {
        Logger::instance().info("[LobbyServer] Room " + std::to_string(roomId) + " was deleted (owner left)");
        instanceManager_.destroyInstance(roomId);
    }
}

std::string LobbyServer::endpointToKey(const IpEndpoint& ep) const
{
    return std::to_string(ep.addr[0]) + "." + std::to_string(ep.addr[1]) + "." + std::to_string(ep.addr[2]) + "." +
           std::to_string(ep.addr[3]) + ":" + std::to_string(ep.port);
}

void LobbyServer::handleRoomKickPlayer(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                       const IpEndpoint& from)
{
    (void) hdr;

    if (size < PacketHeader::kSize + sizeof(std::uint32_t) + sizeof(std::uint32_t)) {
        Logger::instance().warn("[LobbyServer] Kick player packet too small");
        return;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;

    std::uint32_t roomId = (static_cast<std::uint32_t>(payload[0]) << 24) |
                           (static_cast<std::uint32_t>(payload[1]) << 16) |
                           (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);

    std::uint32_t targetPlayerId =
        (static_cast<std::uint32_t>(payload[4]) << 24) | (static_cast<std::uint32_t>(payload[5]) << 16) |
        (static_cast<std::uint32_t>(payload[6]) << 8) | static_cast<std::uint32_t>(payload[7]);

    std::string key           = endpointToKey(from);
    std::uint32_t requesterId = 0;

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        auto it = lobbySessions_.find(key);
        if (it == lobbySessions_.end()) {
            Logger::instance().warn("[LobbyServer] Kick request from unknown session");
            return;
        }
        requesterId = it->second.playerId;
    }

    auto roomInfo = lobbyManager_.getRoomInfo(roomId);
    if (!roomInfo.has_value()) {
        Logger::instance().warn("[LobbyServer] Kick request for non-existent room " + std::to_string(roomId));
        return;
    }

    if (roomInfo->ownerId != requesterId) {
        Logger::instance().warn("[LobbyServer] Player " + std::to_string(requesterId) +
                                " attempted to kick from room " + std::to_string(roomId) + " but is not owner");
        return;
    }

    Logger::instance().info("[LobbyServer] Owner " + std::to_string(requesterId) + " kicking player " +
                            std::to_string(targetPlayerId) + " from room " + std::to_string(roomId));

    IpEndpoint kickedPlayerEndpoint{};
    bool foundPlayer = false;

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        for (const auto& [key, session] : lobbySessions_) {
            if (session.playerId == targetPlayerId) {
                std::string keyStr = key;
                size_t colonPos    = keyStr.rfind(':');
                if (colonPos != std::string::npos) {
                    std::string ipStr   = keyStr.substr(0, colonPos);
                    std::string portStr = keyStr.substr(colonPos + 1);

                    std::istringstream iss(ipStr);
                    std::string octet;
                    int i = 0;
                    while (std::getline(iss, octet, '.') && i < 4) {
                        kickedPlayerEndpoint.addr[i++] = static_cast<std::uint8_t>(std::stoi(octet));
                    }
                    kickedPlayerEndpoint.port = static_cast<std::uint16_t>(std::stoi(portStr));
                    foundPlayer               = true;
                }
                break;
            }
        }
    }

    lobbyManager_.removePlayerFromRoom(roomId, targetPlayerId);

    if (foundPlayer) {
        PacketHeader notifyHdr{};
        notifyHdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        notifyHdr.messageType = static_cast<std::uint8_t>(MessageType::RoomPlayerKicked);
        notifyHdr.sequenceId  = 0;
        notifyHdr.payloadSize = 0;

        auto encoded = notifyHdr.encode();
        std::vector<std::uint8_t> packet(encoded.begin(), encoded.end());

        auto crc = PacketHeader::crc32(packet.data(), packet.size());
        packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

        sendPacket(packet, kickedPlayerEndpoint);
        Logger::instance().info("[LobbyServer] Sent kick notification to player " + std::to_string(targetPlayerId));
    }

    Logger::instance().info("[LobbyServer] Sent kick notification to player " + std::to_string(targetPlayerId));
}

void LobbyServer::handleRoomSetConfig(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                      const IpEndpoint& from)
{
    (void) hdr;

    const std::size_t expectedPayload = sizeof(std::uint32_t) + 1 + sizeof(std::uint16_t) * 3 + 1;
    if (hdr.payloadSize != expectedPayload || size < PacketHeader::kSize + expectedPayload) {
        Logger::instance().warn("[LobbyServer] RoomSetConfig packet bad size");
        return;
    }

    const std::uint8_t* payload = data + PacketHeader::kSize;
    std::uint32_t roomId        = (static_cast<std::uint32_t>(payload[0]) << 24) |
                           (static_cast<std::uint32_t>(payload[1]) << 16) |
                           (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);

    RoomConfig cfg;
    cfg.mode = static_cast<RoomDifficulty>(payload[4]);

    auto decodePercent = [](const std::uint8_t* p) {
        std::uint16_t v = (static_cast<std::uint16_t>(p[0]) << 8) | static_cast<std::uint16_t>(p[1]);
        return static_cast<float>(v) / 100.0F;
    };

    cfg.enemyStatMultiplier   = decodePercent(payload + 5);
    cfg.playerSpeedMultiplier = decodePercent(payload + 7);
    cfg.scoreMultiplier       = decodePercent(payload + 9);
    cfg.playerLives           = payload[11];

    if (cfg.mode == RoomDifficulty::Custom) {
        cfg.clampCustom();
    } else {
        cfg = RoomConfig::preset(cfg.mode);
    }

    std::uint32_t senderRoomId = 0;
    std::uint32_t senderId     = 0;
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        auto it = lobbySessions_.find(endpointToKey(from));
        if (it != lobbySessions_.end()) {
            senderRoomId = it->second.roomId;
            senderId     = it->second.playerId;
        }
    }

    if (senderRoomId == 0 || senderRoomId != roomId) {
        Logger::instance().warn("[LobbyServer] RoomSetConfig from non-room or mismatched room");
        return;
    }

    auto roomInfo = lobbyManager_.getRoomInfo(roomId);
    if (!roomInfo.has_value() || roomInfo->ownerId != senderId) {
        Logger::instance().warn("[LobbyServer] RoomSetConfig from non-owner");
        return;
    }

    Logger::instance().info("[LobbyServer][Config] Applying config to room " + std::to_string(roomId) +
                            " (sender=" + std::to_string(senderId) + ")");
    lobbyManager_.setRoomConfig(roomId, cfg);
    instanceManager_.setRoomConfig(roomId, cfg);
    PacketHeader outHdr{};
    outHdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    outHdr.messageType = static_cast<std::uint8_t>(MessageType::RoomSetConfig);
    outHdr.sequenceId  = hdr.sequenceId;
    outHdr.payloadSize = static_cast<std::uint16_t>(expectedPayload);

    auto pkt              = buildRoomConfigPacket(roomId, cfg, outHdr.sequenceId);
    std::string senderKey = endpointToKey(from);
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    for (const auto& [key, session] : lobbySessions_) {
        if (session.roomId == roomId && key != senderKey) {
            sendPacket(pkt, session.endpoint);
        }
    }
}

void LobbyServer::handleRoomSetReady(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                     const IpEndpoint& from)
{
    if (size < PacketHeader::kSize + sizeof(std::uint32_t) + 1) {
        return;
    }
    const std::uint8_t* payload = data + PacketHeader::kSize;
    std::uint32_t roomId        = (static_cast<std::uint32_t>(payload[0]) << 24) |
                           (static_cast<std::uint32_t>(payload[1]) << 16) |
                           (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);
    bool ready = payload[4] != 0;

    std::uint32_t playerId = 0;
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        auto it = lobbySessions_.find(endpointToKey(from));
        if (it != lobbySessions_.end()) {
            playerId = it->second.playerId;
        }
    }
    if (playerId == 0)
        return;

    lobbyManager_.setPlayerReady(roomId, playerId, ready);

    if (lobbyManager_.isRoomAllReady(roomId)) {
        auto players = lobbyManager_.getRoomPlayers(roomId);
        if (players.size() >= 1) {
            Logger::instance().info("[LobbyServer] At least 1 player ready in room " + std::to_string(roomId) +
                                    ", starting game.");
            handleRoomForceStart(hdr, data, size, from);
        }
    }
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

    std::string token;
    std::string requesterKey = endpointToKey(from);

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        for (const auto& [sessionKey, session] : lobbySessions_) {
            if (session.authenticated && session.userId.has_value() && session.userId.value() == user->id &&
                sessionKey != requesterKey) {
                Logger::instance().warn("[LobbyServer] Login failed: account already connected - " +
                                        loginData->username);
                auto response = buildLoginResponsePacket(false, 0, "", AuthErrorCode::AlreadyConnected, hdr.sequenceId);
                sendPacket(response, from);
                return;
            }
        }

        token                 = authService_->generateJWT(user->id, user->username);
        auto& session         = lobbySessions_[requesterKey];
        session.authenticated = true;
        session.userId        = user->id;
        session.username      = user->username;
        session.playerName    = user->username;
        session.jwtToken      = token;
        session.endpoint      = from;
        session.lastActivity  = std::chrono::steady_clock::now();

        auto stats = userRepository_->getUserStats(user->id);
        if (stats.has_value()) {
            session.elo = stats->elo;
        } else {
            session.elo = 1000;
        }
    }

    userRepository_->updateLastLogin(user->id);

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
        emptyStats.userId = session.userId.value();
        std::strncpy(emptyStats.username, session.username.c_str(), 31);
        emptyStats.username[31] = '\0';
        emptyStats.gamesPlayed  = 0;
        emptyStats.wins         = 0;
        emptyStats.losses       = 0;
        emptyStats.totalScore   = 0;

        auto response = buildGetStatsResponsePacket(emptyStats, hdr.sequenceId);
        sendPacket(response, from);
        return;
    }

    GetStatsResponseData responseData;
    responseData.userId = stats->userId;
    std::strncpy(responseData.username, session.username.c_str(), 31);
    responseData.username[31] = '\0';
    responseData.gamesPlayed  = stats->gamesPlayed;
    responseData.wins         = stats->wins;
    responseData.losses       = stats->losses;
    responseData.totalScore   = stats->totalScore;

    Logger::instance().info("[LobbyServer] Sending stats for user " + session.username +
                            ": games=" + std::to_string(stats->gamesPlayed) + ", wins=" + std::to_string(stats->wins));

    auto response = buildGetStatsResponsePacket(responseData, hdr.sequenceId);
    sendPacket(response, from);
}

void LobbyServer::handleChatPacket(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                   const IpEndpoint& from)
{
    auto chatPkt = ChatPacket::decode(data, size);
    if (!chatPkt.has_value()) {
        Logger::instance().warn("[LobbyServer] Failed to decode ChatPacket from " + endpointToKey(from));
        return;
    }

    std::uint32_t roomId = chatPkt->roomId;
    std::string key      = endpointToKey(from);
    std::uint32_t playerId;
    std::string playerName;

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        if (!lobbySessions_.contains(key)) {
            Logger::instance().warn("[LobbyServer] Chat message from unknown session: " + key);
            return;
        }
        auto& session = lobbySessions_[key];
        if (session.roomId != roomId) {
            Logger::instance().warn("[LobbyServer] Player " + std::to_string(session.playerId) +
                                    " tried to chat in room " + std::to_string(roomId) + " but is in room " +
                                    std::to_string(session.roomId));
            return;
        }
        playerId = session.playerId;
        if (!session.username.empty()) {
            playerName = session.username;
        } else if (!session.playerName.empty()) {
            playerName = session.playerName;
        } else {
            playerName = "Player " + std::to_string(playerId);
        }
    }

    Logger::instance().info("[LobbyServer] Chat in Room " + std::to_string(roomId) + " [" + playerName +
                            "]: " + chatPkt->message);

    if (!rtype::utils::isSafeChatMessage(chatPkt->message)) {
        Logger::instance().warn("[LobbyServer] Rejected unsafe chat message from " + playerName);
        return;
    }

    ChatPacket broadcastPkt;
    broadcastPkt.roomId   = roomId;
    broadcastPkt.playerId = playerId;
    std::strncpy(broadcastPkt.playerName, playerName.c_str(), 31);
    broadcastPkt.playerName[31] = '\0';
    std::memcpy(broadcastPkt.message, chatPkt->message, sizeof(broadcastPkt.message) - 1);
    broadcastPkt.message[sizeof(broadcastPkt.message) - 1] = '\0';

    auto encoded = broadcastPkt.encode(hdr.sequenceId);
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    for (const auto& [sessKey, session] : lobbySessions_) {
        if (session.roomId == roomId) {
            sendPacket(encoded, session.endpoint);
        }
    }
}

void LobbyServer::handleLeaderboardRequest(const PacketHeader& hdr, const IpEndpoint& from)
{
    Logger::instance().info("[LobbyServer] Leaderboard request from " + endpointToKey(from));

    auto topEloRows   = userRepository_->getTopElo(5);
    auto topScoreRows = userRepository_->getTopScore(5);

    LeaderboardResponseData response;
    for (const auto& row : topEloRows) {
        LeaderboardEntry entry{};
        std::strncpy(entry.username, row.username.c_str(), sizeof(entry.username) - 1);
        entry.value = row.value;
        response.topElo.push_back(entry);
    }

    for (const auto& row : topScoreRows) {
        LeaderboardEntry entry{};
        std::strncpy(entry.username, row.username.c_str(), sizeof(entry.username) - 1);
        entry.value = row.value;
        response.topScore.push_back(entry);
    }

    auto packet = buildLeaderboardResponsePacket(response, hdr.sequenceId);
    sendPacket(packet, from);
}
