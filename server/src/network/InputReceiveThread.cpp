#include "network/InputReceiveThread.hpp"

#include "Logger.hpp"

#include <array>
#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
    constexpr std::chrono::milliseconds kPollDelay(1);
    std::string endpointToString(const IpEndpoint& ep)
    {
        std::ostringstream ss;
        ss << static_cast<int>(ep.addr[0]) << '.' << static_cast<int>(ep.addr[1]) << '.' << static_cast<int>(ep.addr[2])
           << '.' << static_cast<int>(ep.addr[3]) << ':' << ep.port;
        return ss.str();
    }
    std::string parseStatusToString(InputParseStatus status)
    {
        switch (status) {
            case InputParseStatus::Ok:
                return "ok";
            case InputParseStatus::DecodeFailed:
                return "decode_failed";
            case InputParseStatus::InvalidFlags:
                return "invalid_flags";
        }
        return "unknown";
    }
} // namespace

InputReceiveThread::InputReceiveThread(const IpEndpoint& bindTo, ThreadSafeQueue<ReceivedInput>& outQueue,
                                       ThreadSafeQueue<ControlEvent>& controlQueue,
                                       ThreadSafeQueue<ClientTimeoutEvent>* timeoutQueue,
                                       std::chrono::milliseconds timeout)
    : bind_(bindTo), queue_(outQueue), controlQueue_(controlQueue), timeoutQueue_(timeoutQueue), timeout_(timeout)
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
    : InputReceiveThread(bindTo, outQueue, *new ThreadSafeQueue<ControlEvent>(), timeoutQueue, timeout)
{}

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
            continue;
        }
        auto hdr = PacketHeader::decode(buf.data(), r.size);
        if (!hdr) {
            Logger::instance().warn("input_drop status=decode_failed from=" + endpointToString(src) +
                                    " size=" + std::to_string(r.size));
            continue;
        }
        if (hdr->packetType != static_cast<std::uint8_t>(PacketType::ClientToServer)) {
            continue;
        }

        if (hdr->messageType == static_cast<std::uint8_t>(MessageType::Input)) {
            EndpointKey key{src.addr, src.port};
            bool stale = false;
            {
                std::lock_guard<std::mutex> lock(sessionMutex_);
                auto it = sessions_.find(key);
                if (it != sessions_.end() && hdr->sequenceId <= it->second.lastSequenceId)
                    stale = true;
            }
            if (stale) {
                Logger::instance().warn("input_drop status=stale_sequence from=" + endpointToString(src) +
                                        " size=" + std::to_string(r.size));
                continue;
            }
            auto parsed = parseInputPacket(buf.data(), r.size);
            if (!parsed.input) {
                Logger::instance().warn("input_drop status=" + parseStatusToString(parsed.status) +
                                        " from=" + endpointToString(src) + " size=" + std::to_string(r.size));
                continue;
            }
            now = std::chrono::steady_clock::now();
            {
                std::lock_guard<std::mutex> lock(sessionMutex_);
                auto& state          = sessions_[key];
                state.lastSequenceId = parsed.input->sequenceId;
                state.lastPacketTime = now;
                lastAccepted_        = key;
            }
            ReceivedInput ev{*parsed.input, src};
            queue_.push(std::move(ev));
            continue;
        }

        if (hdr->messageType == static_cast<std::uint8_t>(MessageType::ClientHello) ||
            hdr->messageType == static_cast<std::uint8_t>(MessageType::ClientJoinRequest) ||
            hdr->messageType == static_cast<std::uint8_t>(MessageType::ClientReady) ||
            hdr->messageType == static_cast<std::uint8_t>(MessageType::ClientPing) ||
            hdr->messageType == static_cast<std::uint8_t>(MessageType::ClientDisconnect)) {
            ControlEvent ev{};
            ev.header = *hdr;
            ev.from   = src;
            controlQueue_.push(std::move(ev));
            continue;
        }

        Logger::instance().warn("input_drop status=decode_failed from=" + endpointToString(src) +
                                " size=" + std::to_string(r.size));
    }
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
