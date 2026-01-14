#include "network/InputReceiveThread.hpp"

#include "Logger.hpp"
#include "core/Session.hpp"

#include <array>
#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
    constexpr std::chrono::milliseconds kPollDelay(1);

    std::string parseStatusToString(InputParseStatus status)
    {
        switch (status) {
            case InputParseStatus::Ok:
                return "ok";
            case InputParseStatus::DecodeFailed:
                return "decode_failed";
            case InputParseStatus::InvalidFlags:
                return "invalid_flags";
            default:
                return "unknown";
        }
    }

    constexpr std::uint16_t kSeqHalfRange = 0x8000;

    bool isNewerSequence(std::uint16_t current, std::uint16_t previous)
    {
        if (current == previous)
            return false;
        return static_cast<std::uint16_t>(current - previous) < kSeqHalfRange;
    }
} // namespace

InputReceiveThread::InputReceiveThread(const IpEndpoint& bindTo, ThreadSafeQueue<ReceivedInput>& outQueue,
                                       ThreadSafeQueue<ControlEvent>& controlQueue,
                                       ThreadSafeQueue<ClientTimeoutEvent>* timeoutQueue,
                                       std::chrono::milliseconds timeout)
    : bind_(bindTo), ownedControlQueue_(nullptr), queue_(outQueue), controlQueue_(controlQueue),
      timeoutQueue_(timeoutQueue), timeout_(timeout)
{
    lastTimeoutCheck_ = std::chrono::steady_clock::now();
}

InputReceiveThread::~InputReceiveThread()
{
    stop();
}

InputReceiveThread::InputReceiveThread(const IpEndpoint& bindTo, ThreadSafeQueue<ReceivedInput>& outQueue,
                                       ThreadSafeQueue<ClientTimeoutEvent>* timeoutQueue,
                                       std::chrono::milliseconds timeout)
    : bind_(bindTo), ownedControlQueue_(std::make_unique<ThreadSafeQueue<ControlEvent>>()), queue_(outQueue),
      controlQueue_(*ownedControlQueue_), timeoutQueue_(timeoutQueue), timeout_(timeout)
{
    lastTimeoutCheck_ = std::chrono::steady_clock::now();
}

bool InputReceiveThread::start()
{
    if (running_)
        return false;
    if (!socket_.open(bind_))
        return false;
    lastTimeoutCheck_ = std::chrono::steady_clock::now();
    running_          = true;
    worker_           = std::thread([this] { run(); });
    return true;
}

void InputReceiveThread::stop()
{
    if (!running_)
        return;
    running_ = false;
    if (worker_.joinable())
        worker_.join();
    socket_.close();
}

bool InputReceiveThread::isRunning() const
{
    return running_;
}

IpEndpoint InputReceiveThread::endpoint() const
{
    return socket_.localEndpoint();
}

std::optional<ClientState> InputReceiveThread::clientState(const IpEndpoint& ep) const
{
    EndpointKey key{ep.addr, ep.port};
    std::lock_guard<std::mutex> lock(sessionMutex_);
    auto it = sessions_.find(key);
    if (it != sessions_.end())
        return it->second;
    if (lastAccepted_.has_value() && ep.addr == socket_.localEndpoint().addr &&
        ep.port == socket_.localEndpoint().port) {
        auto it2 = sessions_.find(*lastAccepted_);
        if (it2 != sessions_.end())
            return it2->second;
    }
    return std::nullopt;
}

void InputReceiveThread::run()
{
    std::array<std::uint8_t, 1024> buf{};
    while (running_) {
        auto now = std::chrono::steady_clock::now();
        if (timeoutQueue_ && now - lastTimeoutCheck_ >= timeout_) {
            checkTimeouts(now);
            lastTimeoutCheck_ = now;
        }

        IpEndpoint src{};
        auto r = socket_.recvFrom(buf.data(), buf.size(), src);
        if (!r.ok()) {
            if (r.error == UdpError::WouldBlock) {
                std::this_thread::sleep_for(kPollDelay);
                continue;
            }
            if (r.error == UdpError::Interrupted)
                continue;

            Logger::instance().warn("[Input] recvFrom failed: error=" + std::to_string(static_cast<int>(r.error)));
            continue;
        }

        processIncomingPacket(buf.data(), r.size, src);
    }
}

void InputReceiveThread::processIncomingPacket(const std::uint8_t* data, std::size_t size, const IpEndpoint& src)
{
    Logger::instance().addBytesReceived(size);
    Logger::instance().addPacketReceived();

    if (size < PacketHeader::kSize) {
        Logger::instance().addPacketDropped();
        Logger::instance().warn("[Input] input_drop status=decode_failed from=" + endpointKey(src) +
                                " size=" + std::to_string(size));
        return;
    }

    auto hdr = PacketHeader::decode(data, size);
    if (!hdr) {
        Logger::instance().addPacketDropped();
        Logger::instance().warn("[Input] input_drop status=decode_failed from=" + endpointKey(src) +
                                " size=" + std::to_string(size));
        return;
    }

    if (hdr->packetType != static_cast<std::uint8_t>(PacketType::ClientToServer))
        return;

    if (hdr->messageType == static_cast<std::uint8_t>(MessageType::Input)) {
        handleInputPacket(*hdr, data, size, src);
    } else {
        handleControlPacket(*hdr, data, size, src);
    }
}

void InputReceiveThread::handleInputPacket(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                           const IpEndpoint& src)
{
    EndpointKey key{src.addr, src.port};
    bool stale = false;
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        auto it = sessions_.find(key);
        if (it != sessions_.end() && it->second.lastSequenceId != 0 &&
            !isNewerSequence(hdr.sequenceId, it->second.lastSequenceId))
            stale = true;
    }

    if (stale) {
        Logger::instance().addPacketDropped();
        Logger::instance().warn("[Input] input_drop status=stale_sequence from=" + endpointKey(src));
        return;
    }

    auto parsed = parseInputPacket(data, size);
    if (parsed.status != InputParseStatus::Ok) {
        Logger::instance().addPacketDropped();
        Logger::instance().warn("[Input] input_drop status=" + parseStatusToString(parsed.status) +
                                " from=" + endpointKey(src));
        return;
    }

    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        auto& state = sessions_[key];
        if (state.lastSequenceId == 0 || isNewerSequence(parsed.input->sequenceId, state.lastSequenceId)) {
            state.lastSequenceId = parsed.input->sequenceId;
        }
        state.lastPacketTime = std::chrono::steady_clock::now();
        lastAccepted_        = key;
    }
    queue_.push(ReceivedInput{*parsed.input, src});
}

void InputReceiveThread::handleControlPacket(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                                             const IpEndpoint& src)
{
    const std::size_t expectedSize = PacketHeader::kSize + hdr.payloadSize + PacketHeader::kCrcSize;
    if (size < expectedSize) {
        Logger::instance().addPacketDropped();
        Logger::instance().warn("[Input] input_drop status=short_control from=" + endpointKey(src) +
                                " size=" + std::to_string(size) + " expected=" + std::to_string(expectedSize));
        return;
    }
    const std::size_t crcOffset   = PacketHeader::kSize + hdr.payloadSize;
    std::uint32_t transmittedCrc  = (static_cast<std::uint32_t>(data[crcOffset]) << 24) |
                                   (static_cast<std::uint32_t>(data[crcOffset + 1]) << 16) |
                                   (static_cast<std::uint32_t>(data[crcOffset + 2]) << 8) |
                                   static_cast<std::uint32_t>(data[crcOffset + 3]);
    std::uint32_t computedControl = PacketHeader::crc32(data, crcOffset);
    if (computedControl != transmittedCrc) {
        Logger::instance().addPacketDropped();
        Logger::instance().warn("[Input] input_drop status=crc_mismatch from=" + endpointKey(src));
        return;
    }
    if (hdr.messageType == static_cast<std::uint8_t>(MessageType::ClientHello) ||
        hdr.messageType == static_cast<std::uint8_t>(MessageType::ClientJoinRequest) ||
        hdr.messageType == static_cast<std::uint8_t>(MessageType::ClientReady) ||
        hdr.messageType == static_cast<std::uint8_t>(MessageType::ClientPing) ||
        hdr.messageType == static_cast<std::uint8_t>(MessageType::RoomForceStart) ||
        hdr.messageType == static_cast<std::uint8_t>(MessageType::RoomSetPlayerCount) ||
        hdr.messageType == static_cast<std::uint8_t>(MessageType::ClientDisconnect)) {
        std::vector<std::uint8_t> packetData(data, data + size);
        controlQueue_.push(ControlEvent{hdr, src, packetData});
    } else {
        Logger::instance().addPacketDropped();
        Logger::instance().warn("[Input] input_drop status=decode_failed from=" + endpointKey(src));
        return;
    }

    EndpointKey key{src.addr, src.port};
    std::lock_guard<std::mutex> lock(sessionMutex_);
    sessions_[key].lastPacketTime = std::chrono::steady_clock::now();
}

void InputReceiveThread::checkTimeouts(std::chrono::steady_clock::time_point now)
{
    if (!timeoutQueue_)
        return;
    std::vector<ClientTimeoutEvent> events;
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if (now - it->second.lastPacketTime >= timeout_) {
                ClientTimeoutEvent ev{};
                ev.endpoint.addr  = it->first.addr;
                ev.endpoint.port  = it->first.port;
                ev.lastSequenceId = it->second.lastSequenceId;
                ev.lastPacketTime = it->second.lastPacketTime;
                events.push_back(ev);
                it = sessions_.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto& ev : events) {
        timeoutQueue_->push(std::move(ev));
    }
}
