#include "network/NetworkMessageHandler.hpp"

#include "network/LevelInitParser.hpp"

NetworkMessageHandler::NetworkMessageHandler(ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue,
                                             ThreadSafeQueue<SnapshotParseResult>& snapshotQueue,
                                             ThreadSafeQueue<LevelInitData>& levelInitQueue,
                                             std::atomic<bool>* handshakeFlag)
    : rawQueue_(rawQueue), snapshotQueue_(snapshotQueue), levelInitQueue_(levelInitQueue),
      handshakeFlag_(handshakeFlag)
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

void NetworkMessageHandler::dispatch(const std::vector<std::uint8_t>& data)
{
    auto hdr = decodeHeader(data);
    if (!hdr.has_value()) {
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
