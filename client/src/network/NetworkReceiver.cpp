#include "network/NetworkReceiver.hpp"

#include <array>
#include <chrono>
#include <thread>

NetworkReceiver::NetworkReceiver(const IpEndpoint& bindEndpoint, SnapshotHandler handler,
                                 std::shared_ptr<UdpSocket> sharedSocket)
    : bindEndpoint_(bindEndpoint), handler_(std::move(handler)), socket_(std::move(sharedSocket))
{}

NetworkReceiver::~NetworkReceiver()
{
    stop();
}

bool NetworkReceiver::start()
{
    if (running_) {
        return false;
    }
    stopRequested_ = false;
    if (socket_ == nullptr) {
        socket_     = std::make_shared<UdpSocket>();
        ownsSocket_ = true;
    }
    if (!socket_->isOpen()) {
        if (!socket_->open(bindEndpoint_)) {
            return false;
        }
    }
    actualEndpoint_ = socket_->localEndpoint();
    running_        = true;
    thread_         = std::thread(&NetworkReceiver::loop, this);
    return true;
}

void NetworkReceiver::stop()
{
    stopRequested_ = true;
    if (thread_.joinable()) {
        thread_.join();
    }
    if (ownsSocket_ && socket_ != nullptr) {
        socket_->close();
    }
    running_ = false;
}

bool NetworkReceiver::running() const
{
    return running_;
}

IpEndpoint NetworkReceiver::endpoint() const
{
    return actualEndpoint_;
}

void NetworkReceiver::loop()
{
    std::array<std::uint8_t, 2048> buffer{};
    IpEndpoint src{};
    while (!stopRequested_) {
        UdpResult res = socket_->recvFrom(buffer.data(), buffer.size(), src);
        if (!res.ok()) {
            if (res.error == UdpError::WouldBlock || res.error == UdpError::Interrupted) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        if (handlePacket(buffer.data(), res.size)) {
            continue;
        }
    }
    running_ = false;
}

bool NetworkReceiver::handlePacket(const std::uint8_t* data, std::size_t len)
{
    if (data == nullptr || len < PacketHeader::kSize) {
        return false;
    }
    auto hdr = PacketHeader::decode(data, len);
    if (!hdr) {
        return false;
    }
    if (hdr->packetType != static_cast<std::uint8_t>(PacketType::ServerToClient)) {
        return false;
    }
    if (hdr->messageType != static_cast<std::uint8_t>(MessageType::Snapshot) &&
        hdr->messageType != static_cast<std::uint8_t>(MessageType::LevelInit) &&
        hdr->messageType != static_cast<std::uint8_t>(MessageType::ServerHello) &&
        hdr->messageType != static_cast<std::uint8_t>(MessageType::ServerJoinAccept) &&
        hdr->messageType != static_cast<std::uint8_t>(MessageType::SnapshotChunk) &&
        hdr->messageType != static_cast<std::uint8_t>(MessageType::ServerJoinDeny) &&
        hdr->messageType != static_cast<std::uint8_t>(MessageType::EntitySpawn) &&
        hdr->messageType != static_cast<std::uint8_t>(MessageType::EntityDestroyed) &&
        hdr->messageType != static_cast<std::uint8_t>(MessageType::GameStart) &&
        hdr->messageType != static_cast<std::uint8_t>(MessageType::ServerPong)) {
        return false;
    }
    const std::size_t payloadSize = hdr->payloadSize;
    const std::size_t minSize     = PacketHeader::kSize + payloadSize;
    const std::size_t withCrc     = minSize + PacketHeader::kCrcSize;
    if (len < minSize) {
        return false;
    }
    const std::size_t copySize = (len >= withCrc) ? withCrc : minSize;
    std::vector<std::uint8_t> packet(data, data + copySize);
    if (handler_) {
        handler_(std::move(packet));
    }
    return true;
}
