#include "network/InputReceiveThread.hpp"

#include <array>
#include <chrono>
#include <thread>

namespace
{
    constexpr std::chrono::milliseconds kPollDelay(1);
}

InputReceiveThread::InputReceiveThread(const IpEndpoint& bindTo, ThreadSafeQueue<ReceivedInput>& outQueue)
    : bind_(bindTo), queue_(outQueue)
{}

InputReceiveThread::~InputReceiveThread()
{
    stop();
}

bool InputReceiveThread::start()
{
    if (running_)
        return false;
    if (!socket_.open(bind_))
        return false;
    running_ = true;
    worker_  = std::thread([this] { run(); });
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

void InputReceiveThread::run()
{
    std::array<std::uint8_t, 1024> buf{};
    while (running_) {
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
        auto parsed = parseInputPacket(buf.data(), r.size);
        if (!parsed)
            continue;
        EndpointKey key{src.addr, src.port};
        auto it = lastSeq_.find(key);
        if (it != lastSeq_.end() && parsed->sequenceId <= it->second)
            continue;
        lastSeq_[key] = parsed->sequenceId;
        ReceivedInput ev{*parsed, src};
        queue_.push(std::move(ev));
    }
}
