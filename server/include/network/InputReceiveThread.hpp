#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "network/InputParser.hpp"
#include "network/UdpSocket.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <thread>
#include <unordered_map>

struct ReceivedInput
{
    ServerInput input;
    IpEndpoint from;
};

class InputReceiveThread
{
  public:
    InputReceiveThread(const IpEndpoint& bindTo, ThreadSafeQueue<ReceivedInput>& outQueue);
    ~InputReceiveThread();

    bool start();
    void stop();
    bool isRunning() const;
    IpEndpoint endpoint() const;

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
    std::atomic<bool> running_{false};
    std::thread worker_;
    UdpSocket socket_;
    std::unordered_map<EndpointKey, std::uint16_t, EndpointKeyHash> lastSeq_;
};
