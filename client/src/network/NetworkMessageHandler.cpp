#include "network/NetworkMessageHandler.hpp"

#include "Logger.hpp"
#include "network/LevelInitParser.hpp"

NetworkMessageHandler::NetworkMessageHandler(ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue,
                                             ThreadSafeQueue<SnapshotParseResult>& snapshotQueue,
                                             ThreadSafeQueue<LevelInitData>& levelInitQueue,
                                             std::atomic<bool>* handshakeFlag, std::atomic<bool>* allReadyFlag,
                                             std::atomic<int>* countdownValueFlag, std::atomic<bool>* gameStartFlag,
                                             std::atomic<bool>* joinDeniedFlag, std::atomic<bool>* joinAcceptedFlag)
    : rawQueue_(rawQueue), snapshotQueue_(snapshotQueue), levelInitQueue_(levelInitQueue),
      handshakeFlag_(handshakeFlag), allReadyFlag_(allReadyFlag), countdownValueFlag_(countdownValueFlag),
      gameStartFlag_(gameStartFlag), joinDeniedFlag_(joinDeniedFlag), joinAcceptedFlag_(joinAcceptedFlag)
{}

void NetworkMessageHandler::poll()
{
    std::vector<std::uint8_t> data;
    while (rawQueue_.tryPop(data)) {
        dispatch(data);
    }
}

std::optional<PacketHeader> NetworkMessageHandler::decodeHeader(const std::vector<std::uint8_t>& data) const
{
    if (data.size() < PacketHeader::kSize + PacketHeader::kCrcSize) {
        return std::nullopt;
    }
    auto hdr = PacketHeader::decode(data.data(), data.size());
    if (!hdr) {
        return std::nullopt;
    }
    if (hdr->packetType != static_cast<std::uint8_t>(PacketType::ServerToClient)) {
        return std::nullopt;
    }
    return hdr;
}

void NetworkMessageHandler::handleSnapshot(const std::vector<std::uint8_t>& data)
{
    auto parsed = SnapshotParser::parse(data);
    if (!parsed.has_value()) {
        return;
    }
    snapshotQueue_.push(std::move(*parsed));
    if (handshakeFlag_ != nullptr) {
        handshakeFlag_->store(true);
    }
}

void NetworkMessageHandler::handleLevelInit(const std::vector<std::uint8_t>& data)
{
    auto parsed = LevelInitParser::parse(data);
    if (!parsed.has_value()) {
        return;
    }
    levelInitQueue_.push(std::move(*parsed));
    if (handshakeFlag_ != nullptr) {
        handshakeFlag_->store(true);
    }
}

void NetworkMessageHandler::handleCountdownTick(const std::vector<std::uint8_t>& data)
{
    if (data.size() < PacketHeader::kSize + 1 + PacketHeader::kCrcSize) {
        return;
    }
    std::uint8_t value = data[PacketHeader::kSize];
    Logger::instance().info("Countdown tick received: " + std::to_string(value));
    if (countdownValueFlag_ != nullptr) {
        countdownValueFlag_->store(static_cast<int>(value));
    }
}

void NetworkMessageHandler::dispatch(const std::vector<std::uint8_t>& data)
{
    auto hdr = decodeHeader(data);
    if (!hdr.has_value()) {
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::ServerJoinAccept)) {
        Logger::instance().info("Received ServerJoinAccept - connection accepted!");
        if (joinAcceptedFlag_ != nullptr) {
            joinAcceptedFlag_->store(true);
        }
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::ServerJoinDeny)) {
        Logger::instance().warn("Received ServerJoinDeny - connection rejected!");
        if (joinDeniedFlag_ != nullptr) {
            joinDeniedFlag_->store(true);
        }
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::GameStart)) {
        Logger::instance().info("Received GameStart - exiting waiting room");
        if (gameStartFlag_ != nullptr) {
            gameStartFlag_->store(true);
        }
        if (handshakeFlag_ != nullptr) {
            handshakeFlag_->store(true);
        }
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::AllReady)) {
        Logger::instance().info("Received SERVER_ALL_READY - all players are ready");
        if (allReadyFlag_ != nullptr) {
            allReadyFlag_->store(true);
        }
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::CountdownTick)) {
        handleCountdownTick(data);
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::Snapshot)) {
        handleSnapshot(data);
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::LevelInit)) {
        handleLevelInit(data);
        return;
    }
}
