#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "events/ClientTimeoutEvent.hpp"
#include "network/InputParser.hpp"
#include "network/PacketHeader.hpp"
#include "network/UdpSocket.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>

struct ReceivedInput
{
    ServerInput input;
    IpEndpoint from;
};

struct ClientState
{
    std::uint16_t lastSequenceId = 0;
    std::chrono::steady_clock::time_point lastPacketTime{};
};

struct ControlEvent
{
    PacketHeader header{};
    IpEndpoint from{};
    std::vector<std::uint8_t> data{};
};

class InputReceiveThread
{
  public:
    InputReceiveThread(const IpEndpoint& bindTo, ThreadSafeQueue<ReceivedInput>& outQueue,
                       ThreadSafeQueue<ControlEvent>& controlQueue,
                       ThreadSafeQueue<ClientTimeoutEvent>* timeoutQueue = nullptr,
                       std::chrono::milliseconds timeout                 = std::chrono::seconds(5));
    InputReceiveThread(const IpEndpoint& bindTo, ThreadSafeQueue<ReceivedInput>& outQueue,
                       ThreadSafeQueue<ClientTimeoutEvent>* timeoutQueue = nullptr,
                       std::chrono::milliseconds timeout                 = std::chrono::seconds(5));
    ~InputReceiveThread();

    bool start();
    void stop();
    bool isRunning() const;
    IpEndpoint endpoint() const;
    std::optional<ClientState> clientState(const IpEndpoint& ep) const;

  private:
    void run();
    struct EndpointKey
    {
        std::array<std::uint8_t, 4> addr{};
        std::uint16_t port = 0;
        bool operator==(const EndpointKey& o) const
        {
            return addr == o.addr && port == o.port;
        }
    };
    struct EndpointKeyHash
    {
        std::size_t operator()(const EndpointKey& k) const
        {
            std::size_t h = static_cast<std::size_t>(k.port);
            h ^= static_cast<std::size_t>(k.addr[0]) << 24;
            h ^= static_cast<std::size_t>(k.addr[1]) << 16;
            h ^= static_cast<std::size_t>(k.addr[2]) << 8;
            h ^= static_cast<std::size_t>(k.addr[3]);
            return h;
        }
    };

    IpEndpoint bind_;
    ThreadSafeQueue<ReceivedInput>& queue_;
    ThreadSafeQueue<ControlEvent>& controlQueue_;
    ThreadSafeQueue<ClientTimeoutEvent>* timeoutQueue_;
    std::chrono::milliseconds timeout_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    UdpSocket socket_;
    mutable std::mutex sessionMutex_;
    std::unordered_map<EndpointKey, ClientState, EndpointKeyHash> sessions_;
    std::optional<EndpointKey> lastAccepted_;
    std::chrono::steady_clock::time_point lastTimeoutCheck_{};

    void checkTimeouts(std::chrono::steady_clock::time_point now);
    void processIncomingPacket(const std::uint8_t* data, std::size_t size, const IpEndpoint& src);
    void handleInputPacket(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size, const IpEndpoint& src);
    void handleControlPacket(const PacketHeader& hdr, const std::uint8_t* data, std::size_t size,
                             const IpEndpoint& src);
};
