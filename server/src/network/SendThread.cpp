#include "network/SendThread.hpp"

#include "Logger.hpp"
#include "core/Session.hpp"

#include <chrono>
#include <thread>

SendThread::SendThread(const IpEndpoint& bindTo, std::vector<IpEndpoint> clients, double hz, int roomId)
    : bind_(bindTo), clients_(std::move(clients)), hz_(hz), roomId_(roomId)
{}

SendThread::~SendThread()
{
    stop();
}

bool SendThread::start()
{
    if (running_)
        return false;
    if (!socket_.open(bind_))
        return false;
    running_ = true;
    worker_  = std::thread([this] { run(); });
    return true;
}

void SendThread::stop()
{
    if (!running_)
        return;
    running_ = false;
    if (worker_.joinable())
        worker_.join();
    socket_.close();
}

bool SendThread::isRunning() const
{
    return running_;
}

void SendThread::setClients(const std::vector<IpEndpoint>& clients)
{
    clients_ = clients;
}

void SendThread::publish(const DeltaStatePacket& packet)
{
    std::lock_guard<std::mutex> lock(payloadMutex_);
    latest_ = packet.encode();
}

void SendThread::publish(const std::vector<std::uint8_t>& payload)
{
    std::lock_guard<std::mutex> lock(payloadMutex_);
    latest_ = payload;
}

void SendThread::sendTo(const std::vector<std::uint8_t>& payload, const IpEndpoint& dst)
{
    if (!running_)
        return;
    socket_.sendTo(payload.data(), payload.size(), dst);
    Logger::instance().addBytesSent(payload.size());
    Logger::instance().addPacketSent();
    Logger::instance().logToRoom(roomId_, "INFO",
                                 "[Packets] Sent " + std::to_string(payload.size()) + " bytes to " + endpointKey(dst));
}

void SendThread::broadcast(const PlayerDisconnectedPacket& packet)
{
    auto payload = packet.encode();
    if (!running_)
        return;
    for (const auto& c : clients_) {
        socket_.sendTo(payload.data(), payload.size(), c);
        Logger::instance().addBytesSent(payload.size());
        Logger::instance().addPacketSent();
        Logger::instance().logToRoom(
            roomId_, "INFO", "[Packets] Broadcasted " + std::to_string(payload.size()) + " bytes to " + endpointKey(c));
    }
}

void SendThread::broadcast(const EntitySpawnPacket& packet)
{
    auto payload = packet.encode();
    if (!running_)
        return;
    for (const auto& c : clients_) {
        socket_.sendTo(payload.data(), payload.size(), c);
        Logger::instance().addBytesSent(payload.size());
        Logger::instance().addPacketSent();
        Logger::instance().logToRoom(
            roomId_, "INFO", "[Packets] Broadcasted " + std::to_string(payload.size()) + " bytes to " + endpointKey(c));
    }
}

void SendThread::broadcast(const EntityDestroyedPacket& packet)
{
    auto payload = packet.encode();
    if (!running_)
        return;
    for (const auto& c : clients_) {
        socket_.sendTo(payload.data(), payload.size(), c);
        Logger::instance().addBytesSent(payload.size());
        Logger::instance().addPacketSent();
        Logger::instance().logToRoom(
            roomId_, "INFO", "[Packets] Broadcasted " + std::to_string(payload.size()) + " bytes to " + endpointKey(c));
    }
}

void SendThread::clearLatest()
{
    std::lock_guard<std::mutex> lock(payloadMutex_);
    latest_.clear();
}

IpEndpoint SendThread::endpoint() const
{
    return socket_.localEndpoint();
}

void SendThread::run()
{
    using namespace std::chrono;
    auto interval = duration<double>(1.0 / hz_);
    auto next     = steady_clock::now() + interval;
    while (running_) {
        std::vector<std::uint8_t> payload;
        {
            std::lock_guard<std::mutex> lock(payloadMutex_);
            payload = latest_;
        }
        if (!payload.empty()) {
            for (const auto& c : clients_) {
                socket_.sendTo(payload.data(), payload.size(), c);
                Logger::instance().addBytesSent(payload.size());
                Logger::instance().addPacketSent();
                Logger::instance().logToRoom(roomId_, "INFO",
                                             "[Packets] Sent " + std::to_string(payload.size()) + " bytes to " +
                                                 endpointKey(c));
            }
        }
        auto now = steady_clock::now();
        if (now < next)
            std::this_thread::sleep_until(next);
        next += interval;
    }
}
