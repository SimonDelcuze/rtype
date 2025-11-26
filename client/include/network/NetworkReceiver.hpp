#pragma once

#include "network/PacketHeader.hpp"
#include "network/UdpSocket.hpp"

#include <atomic>
#include <functional>
#include <thread>
#include <vector>

class NetworkReceiver
{
  public:
    using SnapshotHandler = std::function<void(std::vector<std::uint8_t>&&)>;

    NetworkReceiver(const IpEndpoint& bindEndpoint, SnapshotHandler handler);
    ~NetworkReceiver();

    bool start();
    void stop();
    bool running() const;
    IpEndpoint endpoint() const;

  private:
    void loop();
    bool handlePacket(const std::uint8_t* data, std::size_t len);

    IpEndpoint bindEndpoint_;
    IpEndpoint actualEndpoint_{};
    SnapshotHandler handler_;
    UdpSocket socket_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
};
