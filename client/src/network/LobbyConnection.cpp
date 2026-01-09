#include "network/LobbyConnection.hpp"

#include "Logger.hpp"
#include "network/ServerBroadcastPacket.hpp"
#include "network/ServerDisconnectPacket.hpp"

#include <chrono>
#include <thread>

LobbyConnection::LobbyConnection(const IpEndpoint& lobbyEndpoint, const std::atomic<bool>& runningFlag)
    : lobbyEndpoint_(lobbyEndpoint), runningFlag_(runningFlag)
{}

LobbyConnection::~LobbyConnection()
{
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
        if (hdr.has_value() && hdr->messageType == static_cast<std::uint8_t>(MessageType::ServerBroadcast)) {
            auto pkt = ServerBroadcastPacket::decode(buffer.data(), recvResult.size);
            if (pkt.has_value()) {
                Logger::instance().info("[LobbyConnection] Received broadcast: " + pkt->getMessage());
                broadcastQueue.push(NotificationData{pkt->getMessage(), 5.0F});
            }
        }

        if (hdr.has_value() && hdr->messageType == static_cast<std::uint8_t>(MessageType::ServerDisconnect)) {
            auto pkt = ServerDisconnectPacket::decode(buffer.data(), recvResult.size);
            if (pkt.has_value() && pkt->getReason() == "Server disconnected") {
                Logger::instance().warn("[LobbyConnection] Server disconnected");
                serverLost_ = true;
            }
        }

        if (hdr.has_value() && hdr->messageType == static_cast<std::uint8_t>(MessageType::RoomGameStarting)) {
            if (recvResult.size >= PacketHeader::kSize + sizeof(std::uint32_t) + sizeof(std::uint8_t) + sizeof(std::uint16_t)) {
                const std::uint8_t* payload = buffer.data() + PacketHeader::kSize;
                expectedPlayerCount_ = payload[4];
                Logger::instance().info("[LobbyConnection] Received RoomGameStarting - game is starting with " +
                                       std::to_string(expectedPlayerCount_) + " players!");
                gameStarting_ = true;
            }
        }

        if (hdr.has_value() && hdr->messageType == static_cast<std::uint8_t>(MessageType::RoomPlayerKicked)) {
            Logger::instance().warn("[LobbyConnection] You have been kicked from the room!");
            wasKicked_ = true;
            inRoom_ = false;
        }
    }
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
                                                             const std::string& passwordHash,
                                                             RoomVisibility visibility)
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
    auto packet = buildGetPlayersPacket(roomId, nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::RoomPlayerList, std::chrono::milliseconds(500));

    if (response.empty()) {
        return std::nullopt;
    }

    return parsePlayerListPacket(response.data(), response.size());
}

void LobbyConnection::notifyGameStarting(std::uint32_t roomId)
{
    Logger::instance().info("[LobbyConnection] Notifying server that game is starting for room " + std::to_string(roomId));

    PacketHeader hdr{};
    hdr.packetType = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::RoomForceStart);
    hdr.sequenceId = nextSequence_++;
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
    Logger::instance().info("[LobbyConnection] Kicking player " + std::to_string(playerId) + " from room " + std::to_string(roomId));

    PacketHeader hdr{};
    hdr.packetType = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::RoomKickPlayer);
    hdr.sequenceId = nextSequence_++;
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
    if (!inRoom_) {
        return;
    }

    Logger::instance().info("[LobbyConnection] Sending leave room message");

    PacketHeader hdr{};
    hdr.packetType = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::LobbyLeaveRoom);
    hdr.sequenceId = nextSequence_++;
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
