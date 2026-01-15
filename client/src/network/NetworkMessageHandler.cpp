#include "network/NetworkMessageHandler.hpp"

#include "Logger.hpp"
#include "network/LevelEventParser.hpp"
#include "network/LevelInitParser.hpp"
#include "network/ServerBroadcastPacket.hpp"
#include "network/ServerDisconnectPacket.hpp"
#include "systems/NetworkStatsSystem.hpp"

#include <iostream>

NetworkMessageHandler::NetworkMessageHandler(
    ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue, ThreadSafeQueue<SnapshotParseResult>& snapshotQueue,
    ThreadSafeQueue<LevelInitData>& levelInitQueue, ThreadSafeQueue<LevelEventData>& levelEventQueue,
    ThreadSafeQueue<EntitySpawnPacket>& spawnQueue, ThreadSafeQueue<EntityDestroyedPacket>& destroyQueue,
    ThreadSafeQueue<std::string>* disconnectQueue, ThreadSafeQueue<NotificationData>* broadcastQueue,
    std::atomic<bool>* handshakeFlag, std::atomic<bool>* allReadyFlag, std::atomic<int>* countdownValueFlag,
    std::atomic<bool>* gameStartFlag, std::atomic<bool>* joinDeniedFlag, std::atomic<bool>* joinAcceptedFlag,
    std::atomic<std::uint32_t>* receivedPlayerIdFlag)
    : rawQueue_(rawQueue), snapshotQueue_(snapshotQueue), levelInitQueue_(levelInitQueue),
      levelEventQueue_(levelEventQueue), spawnQueue_(spawnQueue), destroyQueue_(destroyQueue),
      handshakeFlag_(handshakeFlag), allReadyFlag_(allReadyFlag), countdownValueFlag_(countdownValueFlag),
      gameStartFlag_(gameStartFlag), joinDeniedFlag_(joinDeniedFlag), joinAcceptedFlag_(joinAcceptedFlag),
      receivedPlayerIdFlag_(receivedPlayerIdFlag), disconnectQueue_(disconnectQueue), broadcastQueue_(broadcastQueue),
      lastPacketTime_(std::chrono::steady_clock::now())
{}

namespace
{
    ThreadSafeQueue<EntitySpawnPacket>& dummySpawnQueue()
    {
        static ThreadSafeQueue<EntitySpawnPacket> q;
        return q;
    }
    ThreadSafeQueue<EntityDestroyedPacket>& dummyDestroyQueue()
    {
        static ThreadSafeQueue<EntityDestroyedPacket> q;
        return q;
    }
    ThreadSafeQueue<LevelEventData>& dummyLevelEventQueue()
    {
        static ThreadSafeQueue<LevelEventData> q;
        return q;
    }
} // namespace

NetworkMessageHandler::NetworkMessageHandler(ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue,
                                             ThreadSafeQueue<SnapshotParseResult>& snapshotQueue,
                                             ThreadSafeQueue<LevelInitData>& levelInitQueue)
    : NetworkMessageHandler(rawQueue, snapshotQueue, levelInitQueue, dummyLevelEventQueue(), dummySpawnQueue(),
                            dummyDestroyQueue(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)
{}

void NetworkMessageHandler::poll()
{
    std::vector<std::uint8_t> data;
    while (rawQueue_.tryPop(data)) {
        dispatch(data);
    }
    auto now = std::chrono::steady_clock::now();
    for (auto it = chunkAccumulators_.begin(); it != chunkAccumulators_.end();) {
        if (now - it->second.lastUpdate > std::chrono::seconds(2)) {
            it = chunkAccumulators_.erase(it);
        } else {
            ++it;
        }
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

void NetworkMessageHandler::handleLevelEvent(const std::vector<std::uint8_t>& data)
{
    auto parsed = LevelEventParser::parse(data);
    if (!parsed.has_value()) {
        return;
    }
    levelEventQueue_.push(std::move(*parsed));
}

void NetworkMessageHandler::handleCountdownTick(const std::vector<std::uint8_t>& data)
{
    if (data.size() < PacketHeader::kSize + 1 + PacketHeader::kCrcSize) {
        return;
    }
    std::uint8_t value = data[PacketHeader::kSize];
    if (countdownValueFlag_ != nullptr) {
        countdownValueFlag_->store(static_cast<int>(value));
    }
}

void NetworkMessageHandler::handleSnapshotChunk(const PacketHeader& hdr, const std::vector<std::uint8_t>& data)
{
    if (data.size() < PacketHeader::kSize + 6 + PacketHeader::kCrcSize) {
        return;
    }
    std::size_t offset = PacketHeader::kSize;
    auto readU16       = [&](std::size_t& off) -> std::uint16_t {
        std::uint16_t v = (static_cast<std::uint16_t>(data[off]) << 8) | static_cast<std::uint16_t>(data[off + 1]);
        off += 2;
        return v;
    };
    std::uint16_t totalChunks = readU16(offset);
    std::uint16_t chunkIndex  = readU16(offset);
    std::uint16_t entityCount = readU16(offset);
    if (totalChunks == 0)
        return;

    auto& acc = chunkAccumulators_[hdr.tickId];
    if (acc.totalChunks == 0) {
        acc.totalChunks                = totalChunks;
        acc.headerTemplate             = hdr;
        acc.headerTemplate.messageType = static_cast<std::uint8_t>(MessageType::Snapshot);
        acc.parts.resize(totalChunks);
    }
    if (chunkIndex >= acc.parts.size())
        return;
    if (acc.parts[chunkIndex].empty()) {
        acc.parts[chunkIndex] = std::vector<std::uint8_t>(data.begin() + offset, data.end() - PacketHeader::kCrcSize);
        acc.totalEntities += entityCount;
        acc.received++;
    }
    acc.lastUpdate = std::chrono::steady_clock::now();

    if (acc.received != acc.totalChunks) {
        return;
    }
    std::vector<std::uint8_t> payload;
    payload.push_back(static_cast<std::uint8_t>((acc.totalEntities >> 8) & 0xFF));
    payload.push_back(static_cast<std::uint8_t>(acc.totalEntities & 0xFF));
    for (std::size_t i = 0; i < acc.parts.size(); ++i) {
        payload.insert(payload.end(), acc.parts[i].begin(), acc.parts[i].end());
    }

    PacketHeader outHdr = acc.headerTemplate;
    outHdr.payloadSize  = static_cast<std::uint16_t>(payload.size());
    auto hdrBytes       = outHdr.encode();
    std::vector<std::uint8_t> packet(hdrBytes.begin(), hdrBytes.end());
    packet.insert(packet.end(), payload.begin(), payload.end());
    auto crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    chunkAccumulators_.erase(hdr.tickId);
    handleSnapshot(packet);
}

void NetworkMessageHandler::handleEntitySpawn(const std::vector<std::uint8_t>& data)
{
    auto pkt = EntitySpawnPacket::decode(data.data(), data.size());
    if (pkt.has_value()) {
        spawnQueue_.push(*pkt);
    }
}

void NetworkMessageHandler::handleEntityDestroyed(const std::vector<std::uint8_t>& data)
{
    auto pkt = EntityDestroyedPacket::decode(data.data(), data.size());
    if (pkt.has_value()) {
        destroyQueue_.push(*pkt);
    }
}

void NetworkMessageHandler::handleServerDisconnect(const std::vector<std::uint8_t>& data)
{
    auto pkt = ServerDisconnectPacket::decode(data.data(), data.size());
    if (pkt.has_value()) {
        if (disconnectQueue_ != nullptr) {
            disconnectQueue_->push(pkt->getReason());
        }
        if (broadcastQueue_ != nullptr) {
            broadcastQueue_->push(NotificationData{pkt->getReason(), 5.0F});
        }
    }
}

void NetworkMessageHandler::dispatch(const std::vector<std::uint8_t>& data)
{
    auto hdr = decodeHeader(data);
    if (!hdr.has_value()) {
        return;
    }
    lastPacketTime_ = std::chrono::steady_clock::now();
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::ServerJoinAccept)) {
        if (data.size() >= PacketHeader::kSize + sizeof(std::uint32_t) + PacketHeader::kCrcSize) {
            const std::uint8_t* payload = data.data() + PacketHeader::kSize;
            std::uint32_t playerId =
                (static_cast<std::uint32_t>(payload[0]) << 24) | (static_cast<std::uint32_t>(payload[1]) << 16) |
                (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);
            if (receivedPlayerIdFlag_ != nullptr) {
                receivedPlayerIdFlag_->store(playerId);
                Logger::instance().info("[NetworkMessageHandler] Received playerId from server: " +
                                        std::to_string(playerId));
            }
        }
        if (joinAcceptedFlag_ != nullptr) {
            joinAcceptedFlag_->store(true);
        }
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::ServerJoinDeny)) {
        if (joinDeniedFlag_ != nullptr) {
            joinDeniedFlag_->store(true);
        }
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::GameStart)) {
        if (gameStartFlag_ != nullptr) {
            gameStartFlag_->store(true);
        }
        if (handshakeFlag_ != nullptr) {
            handshakeFlag_->store(true);
        }
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::AllReady)) {
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
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::SnapshotChunk)) {
        handleSnapshotChunk(*hdr, data);
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::EntitySpawn)) {
        handleEntitySpawn(data);
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::EntityDestroyed)) {
        handleEntityDestroyed(data);
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::LevelInit)) {
        handleLevelInit(data);
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::LevelEvent)) {
        handleLevelEvent(data);
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::ServerDisconnect) ||
        hdr->messageType == static_cast<std::uint8_t>(MessageType::ServerKick) ||
        hdr->messageType == static_cast<std::uint8_t>(MessageType::ServerBan)) {
        handleServerDisconnect(data);
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::ServerBroadcast)) {
        handleServerBroadcast(data);
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::GameEnd)) {
        std::cout << ">>> [NETWORK] Dispatching GameEnd Packet! <<<" << std::endl;
        Logger::instance().info("[Network] Received GameEnd packet in dispatch");
        handleGameEnd(data);
        return;
    }
    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::ServerPong)) {
        Logger::instance().info("[Network] Received ServerPong packet");
        recordGlobalPongReceived();
        return;
    }
}

void NetworkMessageHandler::handleGameEnd(const std::vector<std::uint8_t>& data)
{
    auto pkt = GameEndPacket::decode(data.data(), data.size());
    if (pkt) {
        std::cout << ">>> [NETWORK] GameEnd Packet Decoded SUCCESSFULLY. Victory: " << pkt->victory << std::endl;
        Logger::instance().info("[Network] Successfully decoded GameEnd packet. Victory: " +
                                (pkt->victory ? std::string("true") : std::string("false")));
        gameEndQueue_.push(*pkt);
    } else {
        std::cout << ">>> [NETWORK] GameEnd Packet Decode FAILED!" << std::endl;
        Logger::instance().error("[Network] Failed to decode GameEnd packet");
    }
}

void NetworkMessageHandler::handleServerBroadcast(const std::vector<std::uint8_t>& data)
{
    auto pkt = ServerBroadcastPacket::decode(data.data(), data.size());
    if (pkt.has_value() && broadcastQueue_ != nullptr) {
        broadcastQueue_->push(NotificationData{pkt->getMessage(), 5.0F});
    }
}

float NetworkMessageHandler::getLastPacketAge() const
{
    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPacketTime_).count();
    return static_cast<float>(elapsed) / 1000.0F;
}
