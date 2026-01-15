#include "network/LobbyConnection.hpp"

#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "network/LeaderboardPacket.hpp"
#include "network/ServerBroadcastPacket.hpp"
#include "network/ServerDisconnectPacket.hpp"
#include "utils/StringSanity.hpp"

#include <chrono>
#include <thread>

LobbyConnection::LobbyConnection(const IpEndpoint& lobbyEndpoint, const std::atomic<bool>& runningFlag)
    : lobbyEndpoint_(lobbyEndpoint), runningFlag_(runningFlag)
{}

LobbyConnection::~LobbyConnection()
{
    if (g_forceExit.load()) {
        socket_.close();
        return;
    }
    if (gameStarting_) {
        disconnect();
        return;
    }
    if (inRoom_) {
        leaveRoom();
    }
    disconnect();
}

bool LobbyConnection::connect()
{
    IpEndpoint bind{.addr = {0, 0, 0, 0}, .port = 0};

    if (!socket_.open(bind)) {
        return false;
    }

    socket_.setNonBlocking(true);

    return true;
}

void LobbyConnection::disconnect()
{
    if (g_forceExit.load()) {
        socket_.close();
        return;
    }

    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::ClientDisconnect);
    hdr.payloadSize = 0;

    auto encoded = hdr.encode();
    std::vector<std::uint8_t> packet(encoded.begin(), encoded.end());
    auto crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    socket_.close();
}

void LobbyConnection::poll(ThreadSafeQueue<NotificationData>& broadcastQueue)
{
    std::array<std::uint8_t, 2048> buffer{};
    IpEndpoint from{};

    while (true) {
        auto recvResult = socket_.recvFrom(buffer.data(), buffer.size(), from);
        if (!recvResult.ok() || recvResult.size == 0) {
            break;
        }

        auto hdr = PacketHeader::decode(buffer.data(), recvResult.size);
        if (!hdr.has_value())
            continue;

        MessageType type = static_cast<MessageType>(hdr->messageType);

        if (type == MessageType::ServerBroadcast) {
            auto pkt = ServerBroadcastPacket::decode(buffer.data(), recvResult.size);
            if (pkt.has_value()) {
                Logger::instance().info("[LobbyConnection] Received broadcast: " + pkt->getMessage());
                broadcastQueue.push(NotificationData{pkt->getMessage(), 5.0F});
            }
        } else if (type == MessageType::ServerDisconnect) {
            auto pkt = ServerDisconnectPacket::decode(buffer.data(), recvResult.size);
            if (pkt.has_value() && pkt->getReason() == "Server disconnected") {
                Logger::instance().warn("[LobbyConnection] Server disconnected");
                serverLost_ = true;
            }
        } else if (type == MessageType::RoomGameStarting) {
            if (recvResult.size >=
                PacketHeader::kSize + sizeof(std::uint32_t) + sizeof(std::uint8_t) + sizeof(std::uint16_t)) {
                const std::uint8_t* payload = buffer.data() + PacketHeader::kSize;
                expectedPlayerCount_        = payload[4];
                Logger::instance().info("[LobbyConnection] Received RoomGameStarting - game is starting with " +
                                        std::to_string(expectedPlayerCount_) + " players!");
                gameStarting_ = true;
                inRoom_       = false;
            }
        } else if (type == MessageType::RoomPlayerKicked) {
            Logger::instance().warn("[LobbyConnection] You have been kicked from the room!");
            wasKicked_ = true;
            inRoom_    = false;
        } else if (type == MessageType::AuthLoginResponse) {
            auto pkt = parseLoginResponsePacket(buffer.data(), recvResult.size);
            if (pkt.has_value())
                pendingLoginResult_ = pkt;
        } else if (type == MessageType::AuthRegisterResponse) {
            auto pkt = parseRegisterResponsePacket(buffer.data(), recvResult.size);
            if (pkt.has_value())
                pendingRegisterResult_ = pkt;
        } else if (type == MessageType::LobbyRoomList) {
            auto pkt = parseRoomListPacket(buffer.data(), recvResult.size);
            if (pkt.has_value())
                pendingRoomListResult_ = pkt;
        } else if (type == MessageType::LobbyRoomCreated) {
            auto pkt = parseRoomCreatedPacket(buffer.data(), recvResult.size);
            if (pkt.has_value()) {
                pendingRoomCreatedResult_ = pkt;
                inRoom_                   = true;
            }
        } else if (type == MessageType::LobbyJoinSuccess) {
            auto pkt = parseJoinSuccessPacket(buffer.data(), recvResult.size);
            if (pkt.has_value()) {
                pendingJoinRoomResult_ = pkt;
                inRoom_                = true;
            }
        } else if (type == MessageType::LobbyJoinFailed) {
            Logger::instance().warn("[LobbyConnection] Join failed received");
            pendingJoinRoomResult_ = std::nullopt;
        } else if (type == MessageType::RoomPlayerList) {
            if (recvResult.size >= PacketHeader::kSize + 4 + 1 + 1) {
                const std::uint8_t* payload = buffer.data() + PacketHeader::kSize + 4;
                currentRoomCountdown_       = payload[0];
            }
            auto pkt = parsePlayerListPacket(buffer.data(), recvResult.size);
            if (pkt.has_value())
                pendingPlayerListResult_ = pkt;
        } else if (type == MessageType::RoomSetConfig) {
            if (hdr->packetType == static_cast<std::uint8_t>(PacketType::ServerToClient) &&
                hdr->payloadSize >= sizeof(std::uint32_t) + 1 + sizeof(std::uint16_t) * 3 + 1) {
                const std::uint8_t* payload = buffer.data() + PacketHeader::kSize;
                RoomConfigUpdate upd;
                upd.roomId = (static_cast<std::uint32_t>(payload[0]) << 24) |
                             (static_cast<std::uint32_t>(payload[1]) << 16) |
                             (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);
                upd.mode           = static_cast<RoomDifficulty>(payload[4]);
                auto decodePercent = [](const std::uint8_t* p) {
                    std::uint16_t v = (static_cast<std::uint16_t>(p[0]) << 8) | static_cast<std::uint16_t>(p[1]);
                    return static_cast<float>(v) / 100.0F;
                };
                upd.enemyMultiplier       = decodePercent(payload + 5);
                upd.playerSpeedMultiplier = decodePercent(payload + 7);
                upd.scoreMultiplier       = decodePercent(payload + 9);
                upd.playerLives           = payload[11];
                pendingRoomConfig_        = upd;
                Logger::instance().info("[LobbyConnection] Received RoomSetConfig for room " +
                                        std::to_string(upd.roomId));
            }
        } else if (type == MessageType::Chat) {
            auto pkt = ChatPacket::decode(buffer.data(), recvResult.size);
            if (pkt.has_value()) {
                chatMessages_.push(*pkt);
            }
        } else if (type == MessageType::LeaderboardResponse) {
            auto data                 = parseLeaderboardResponsePacket(buffer.data(), recvResult.size);
            pendingLeaderboardResult_ = data;
        }
    }
}

void LobbyConnection::sendNotifyGameStarting(std::uint32_t roomId)
{
    notifyGameStarting(roomId);
}

void LobbyConnection::sendKickPlayer(std::uint32_t roomId, std::uint32_t playerId)
{
    kickPlayer(roomId, playerId);
}

void LobbyConnection::sendRoomConfig(std::uint32_t roomId, RoomDifficulty mode, float enemyMult, float playerSpeedMult,
                                     float scoreMult, std::uint8_t lives)
{
    Logger::instance().info("[LobbyConnection] Sending room config for room " + std::to_string(roomId));

    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::RoomSetConfig);
    hdr.sequenceId  = nextSequence_++;
    hdr.payloadSize = sizeof(std::uint32_t) + 1 + sizeof(std::uint16_t) * 3 + 1;

    auto encoded = hdr.encode();
    std::vector<std::uint8_t> packet(encoded.begin(), encoded.end());

    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    packet.push_back(static_cast<std::uint8_t>(mode));

    auto encodePercent = [](float f) {
        int v = static_cast<int>(std::round(f * 100.0F));
        v     = std::clamp(v, 0, 1000);
        return static_cast<std::uint16_t>(v);
    };
    std::uint16_t enemy  = encodePercent(enemyMult);
    std::uint16_t player = encodePercent(playerSpeedMult);
    std::uint16_t score  = encodePercent(scoreMult);

    packet.push_back(static_cast<std::uint8_t>((enemy >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(enemy & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((player >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(player & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((score >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(score & 0xFF));

    packet.push_back(lives);

    auto crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);
}

void LobbyConnection::sendSetReady(std::uint32_t roomId, bool ready)
{
    auto packet = buildRoomSetReadyPacket(roomId, ready, nextSequence_++);
    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);
}

void LobbyConnection::sendLeaveRoom()
{
    leaveRoom();
}

void LobbyConnection::sendLogin(const std::string& username, const std::string& password)
{
    auto packet = buildLoginRequestPacket(username, password, nextSequence_++);
    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);
    pendingLoginResult_.reset();
}

bool LobbyConnection::hasLoginResult() const
{
    return pendingLoginResult_.has_value();
}

std::optional<LoginResponseData> LobbyConnection::popLoginResult()
{
    auto res = pendingLoginResult_;
    pendingLoginResult_.reset();
    return res;
}

void LobbyConnection::sendRegister(const std::string& username, const std::string& password)
{
    auto packet = buildRegisterRequestPacket(username, password, nextSequence_++);
    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);
    pendingRegisterResult_.reset();
}

bool LobbyConnection::hasRegisterResult() const
{
    return pendingRegisterResult_.has_value();
}

std::optional<RegisterResponseData> LobbyConnection::popRegisterResult()
{
    auto res = pendingRegisterResult_;
    pendingRegisterResult_.reset();
    return res;
}

void LobbyConnection::sendRequestRoomList()
{
    auto packet = buildListRoomsPacket(nextSequence_++);
    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);
    pendingRoomListResult_.reset();
}

bool LobbyConnection::hasRoomListResult() const
{
    return pendingRoomListResult_.has_value();
}

std::optional<RoomListResult> LobbyConnection::popRoomListResult()
{
    auto res = pendingRoomListResult_;
    pendingRoomListResult_.reset();
    return res;
}

void LobbyConnection::sendCreateRoom(const std::string& roomName, const std::string& password,
                                     RoomVisibility visibility)
{
    auto packet = buildCreateRoomPacket(roomName, password, visibility, nextSequence_++);
    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);
    pendingRoomCreatedResult_.reset();
}

bool LobbyConnection::hasRoomCreatedResult() const
{
    return pendingRoomCreatedResult_.has_value();
}

std::optional<RoomCreatedResult> LobbyConnection::popRoomCreatedResult()
{
    auto res = pendingRoomCreatedResult_;
    pendingRoomCreatedResult_.reset();
    return res;
}

void LobbyConnection::sendJoinRoom(std::uint32_t roomId, const std::string& password)
{
    auto packet = buildJoinRoomPacket(roomId, password, nextSequence_++);
    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);
    pendingJoinRoomResult_.reset();
}

bool LobbyConnection::hasJoinRoomResult() const
{
    return pendingJoinRoomResult_.has_value();
}

std::optional<JoinSuccessResult> LobbyConnection::popJoinRoomResult()
{
    auto res = pendingJoinRoomResult_;
    pendingJoinRoomResult_.reset();
    return res;
}

bool LobbyConnection::ping()
{
    auto packet   = buildListRoomsPacket(nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::LobbyRoomList, std::chrono::milliseconds(500));
    return !response.empty();
}

std::optional<RoomListResult> LobbyConnection::requestRoomList()
{
    auto packet   = buildListRoomsPacket(nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::LobbyRoomList);

    if (response.empty()) {
        return std::nullopt;
    }

    return parseRoomListPacket(response.data(), response.size());
}

std::optional<RoomCreatedResult> LobbyConnection::createRoom()
{
    return createRoom("New Room", "", RoomVisibility::Public);
}

std::optional<RoomCreatedResult> LobbyConnection::createRoom(const std::string& roomName,
                                                             const std::string& passwordHash, RoomVisibility visibility)
{
    auto packet   = buildCreateRoomPacket(roomName, passwordHash, visibility, nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::LobbyRoomCreated);

    if (response.empty()) {
        return std::nullopt;
    }

    auto result = parseRoomCreatedPacket(response.data(), response.size());
    if (result.has_value()) {
        inRoom_ = true;
    }
    return result;
}

std::optional<JoinSuccessResult> LobbyConnection::joinRoom(std::uint32_t roomId)
{
    auto packet   = buildJoinRoomPacket(roomId, nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::LobbyJoinSuccess, std::chrono::seconds(5));

    if (response.empty()) {
        return std::nullopt;
    }

    auto result = parseJoinSuccessPacket(response.data(), response.size());
    if (result.has_value()) {
        inRoom_ = true;
    }
    return result;
}

std::optional<JoinSuccessResult> LobbyConnection::joinRoom(std::uint32_t roomId, const std::string& passwordHash)
{
    auto packet   = buildJoinRoomPacket(roomId, passwordHash, nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::LobbyJoinSuccess, std::chrono::seconds(5));

    if (response.empty()) {
        return std::nullopt;
    }

    auto result = parseJoinSuccessPacket(response.data(), response.size());
    if (result.has_value()) {
        inRoom_ = true;
    }
    return result;
}

std::optional<std::vector<PlayerInfo>> LobbyConnection::requestPlayerList(std::uint32_t roomId)
{
    auto packet   = buildGetPlayersPacket(roomId, nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::RoomPlayerList, std::chrono::milliseconds(500));

    if (response.empty()) {
        return std::nullopt;
    }

    return parsePlayerListPacket(response.data(), response.size());
}

void LobbyConnection::notifyGameStarting(std::uint32_t roomId)
{
    Logger::instance().info("[LobbyConnection] Notifying server that game is starting for room " +
                            std::to_string(roomId));

    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::RoomForceStart);
    hdr.sequenceId  = nextSequence_++;
    hdr.payloadSize = sizeof(std::uint32_t);

    auto encoded = hdr.encode();
    std::vector<std::uint8_t> packet(encoded.begin(), encoded.end());

    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    auto crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);
}

void LobbyConnection::kickPlayer(std::uint32_t roomId, std::uint32_t playerId)
{
    Logger::instance().info("[LobbyConnection] Kicking player " + std::to_string(playerId) + " from room " +
                            std::to_string(roomId));

    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::RoomKickPlayer);
    hdr.sequenceId  = nextSequence_++;
    hdr.payloadSize = sizeof(std::uint32_t) + sizeof(std::uint32_t);

    auto encoded = hdr.encode();
    std::vector<std::uint8_t> packet(encoded.begin(), encoded.end());

    packet.push_back(static_cast<std::uint8_t>((roomId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((roomId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(roomId & 0xFF));

    packet.push_back(static_cast<std::uint8_t>((playerId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((playerId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((playerId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(playerId & 0xFF));

    auto crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);
}

void LobbyConnection::leaveRoom()
{
    if (g_forceExit.load()) {
        return;
    }
    if (!inRoom_) {
        return;
    }

    Logger::instance().info("[LobbyConnection] Sending leave room message");

    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyLeaveRoom);
    hdr.sequenceId  = nextSequence_++;
    hdr.payloadSize = 0;

    auto encoded = hdr.encode();
    std::vector<std::uint8_t> packet(encoded.begin(), encoded.end());

    auto crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);

    inRoom_ = false;
}

std::vector<std::uint8_t> LobbyConnection::sendAndWaitForResponse(const std::vector<std::uint8_t>& packet,
                                                                  MessageType expectedResponse,
                                                                  std::chrono::milliseconds timeout)
{
    constexpr int maxRetries = 5;
    const auto retryDelay    = std::chrono::milliseconds(300);

    for (int retry = 0; retry < maxRetries; ++retry) {
        if (!runningFlag_.load()) {
            return {};
        }

        auto sendResult = socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);

        if (!sendResult.ok()) {
            std::this_thread::sleep_for(retryDelay);
            continue;
        }

        auto deadline = std::chrono::steady_clock::now() + timeout;

        while (std::chrono::steady_clock::now() < deadline) {
            if (!runningFlag_.load()) {
                return {};
            }
            std::array<std::uint8_t, 2048> buffer{};
            IpEndpoint from{};

            auto recvResult = socket_.recvFrom(buffer.data(), buffer.size(), from);

            if (recvResult.ok() && recvResult.size > 0) {
                auto hdr = PacketHeader::decode(buffer.data(), recvResult.size);

                if (hdr.has_value() && static_cast<MessageType>(hdr->messageType) == expectedResponse) {
                    return std::vector<std::uint8_t>(buffer.begin(), buffer.begin() + recvResult.size);
                }

                if (hdr.has_value() && static_cast<MessageType>(hdr->messageType) == MessageType::LobbyJoinFailed) {
                    return {};
                }

                if (hdr.has_value() && static_cast<MessageType>(hdr->messageType) == MessageType::ServerDisconnect) {
                    auto discPkt = ServerDisconnectPacket::decode(buffer.data(), recvResult.size);
                    if (discPkt.has_value() && discPkt->getReason() == "Server disconnected") {
                        Logger::instance().warn("[LobbyConnection] Server disconnected while waiting for response");
                        serverLost_ = true;
                        return {};
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    return {};
}

std::optional<LoginResponseData> LobbyConnection::login(const std::string& username, const std::string& password)
{
    auto packet   = buildLoginRequestPacket(username, password, nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::AuthLoginResponse, std::chrono::seconds(5));

    if (response.empty()) {
        return std::nullopt;
    }

    return parseLoginResponsePacket(response.data(), response.size());
}

std::optional<RegisterResponseData> LobbyConnection::registerUser(const std::string& username,
                                                                  const std::string& password)
{
    auto packet   = buildRegisterRequestPacket(username, password, nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::AuthRegisterResponse, std::chrono::seconds(5));

    if (response.empty()) {
        return std::nullopt;
    }

    return parseRegisterResponsePacket(response.data(), response.size());
}

std::optional<ChangePasswordResponseData> LobbyConnection::changePassword(const std::string& oldPassword,
                                                                          const std::string& newPassword,
                                                                          const std::string& token)
{
    auto packet   = buildChangePasswordRequestPacket(oldPassword, newPassword, token, nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::AuthChangePasswordResponse, std::chrono::seconds(5));

    if (response.empty()) {
        return std::nullopt;
    }

    return parseChangePasswordResponsePacket(response.data(), response.size());
}

std::optional<GetStatsResponseData> LobbyConnection::getStats()
{
    Logger::instance().info("[LobbyConnection] Requesting user stats...");
    auto packet   = buildGetStatsRequestPacket(nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::AuthGetStatsResponse, std::chrono::seconds(5));

    if (response.empty()) {
        Logger::instance().warn("[LobbyConnection] No response received for stats request");
        return std::nullopt;
    }

    auto parsed = parseGetStatsResponsePacket(response.data(), response.size());
    if (parsed.has_value()) {
        Logger::instance().info("[LobbyConnection] Stats response parsed successfully");
    } else {
        Logger::instance().warn("[LobbyConnection] Failed to parse stats response");
    }

    return parsed;
}
void LobbyConnection::sendRequestPlayerList(std::uint32_t roomId)
{
    auto packet = buildGetPlayersPacket(roomId, nextSequence_++);
    if (socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_).size != packet.size()) {
        Logger::instance().error("[LobbyConnection] Failed to send player list request");
    }
    pendingPlayerListResult_.reset();
}

bool LobbyConnection::hasPlayerListResult() const
{
    return pendingPlayerListResult_.has_value();
}

std::optional<std::vector<PlayerInfo>> LobbyConnection::popPlayerListResult()
{
    auto res = pendingPlayerListResult_;
    pendingPlayerListResult_.reset();
    return res;
}

void LobbyConnection::sendChatMessage(std::uint32_t roomId, const std::string& message)
{
    ChatPacket pkt;
    pkt.roomId = roomId;

    std::string safeMessage = rtype::utils::sanitizeChatMessage(message);
    if (safeMessage.empty() && !message.empty()) {
        Logger::instance().warn("[LobbyConnection] Message was completely sanitized (unsafe characters)");
        return;
    }

    std::strncpy(pkt.message, safeMessage.c_str(), 120);
    pkt.message[120] = '\0';

    auto encoded = pkt.encode(nextSequence_++);
    socket_.sendTo(encoded.data(), encoded.size(), lobbyEndpoint_);
}

bool LobbyConnection::hasNewChatMessages() const
{
    return !chatMessages_.empty();
}

std::vector<ChatPacket> LobbyConnection::popChatMessages()
{
    std::vector<ChatPacket> messages;
    ChatPacket msg;
    while (chatMessages_.tryPop(msg)) {
        messages.push_back(msg);
    }
    return messages;
}

void LobbyConnection::sendRequestLeaderboard()
{
    auto packet = buildLeaderboardRequestPacket(nextSequence_++);
    socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);
    pendingLeaderboardResult_.reset();
}

bool LobbyConnection::hasLeaderboardResult() const
{
    return pendingLeaderboardResult_.has_value();
}

std::optional<LeaderboardResponseData> LobbyConnection::popLeaderboardResult()
{
    auto res = pendingLeaderboardResult_;
    pendingLeaderboardResult_.reset();
    return res;
}

#include "ClientRuntime.hpp"
