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
    auto packet   = buildCreateRoomPacket(nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::LobbyRoomCreated);

    if (response.empty()) {
        return std::nullopt;
    }

    return parseRoomCreatedPacket(response.data(), response.size());
}

std::optional<JoinSuccessResult> LobbyConnection::joinRoom(std::uint32_t roomId)
{
    auto packet   = buildJoinRoomPacket(roomId, nextSequence_++);
    auto response = sendAndWaitForResponse(packet, MessageType::LobbyJoinSuccess, std::chrono::seconds(5));

    if (response.empty()) {
        return std::nullopt;
    }

    return parseJoinSuccessPacket(response.data(), response.size());
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
