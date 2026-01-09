#include "network/LobbyConnection.hpp"

#include "Logger.hpp"
#include "network/ServerBroadcastPacket.hpp"

#include <chrono>
#include <thread>

LobbyConnection::LobbyConnection(const IpEndpoint& lobbyEndpoint) : lobbyEndpoint_(lobbyEndpoint) {}

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

void LobbyConnection::poll(ThreadSafeQueue<std::string>& broadcastQueue)
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
                broadcastQueue.push(pkt->getMessage());
            }
        }
    }
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
    constexpr int maxRetries = 3;
    const auto retryDelay    = std::chrono::milliseconds(200);

    for (int retry = 0; retry < maxRetries; ++retry) {
        auto sendResult = socket_.sendTo(packet.data(), packet.size(), lobbyEndpoint_);

        if (!sendResult.ok()) {
            std::this_thread::sleep_for(retryDelay);
            continue;
        }

        auto deadline = std::chrono::steady_clock::now() + timeout;

        while (std::chrono::steady_clock::now() < deadline) {
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
