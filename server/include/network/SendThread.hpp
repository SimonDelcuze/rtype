#pragma once

#include "network/DeltaStatePacket.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"
#include "network/PlayerDisconnectedPacket.hpp"
#include "network/UdpSocket.hpp"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

class SendThread
{
  public:
    SendThread(const IpEndpoint& bindTo, std::vector<IpEndpoint> clients, double hz = 60.0);
    ~SendThread();

    bool start();
    void stop();
    bool isRunning() const;
    void setClients(const std::vector<IpEndpoint>& clients);
    void publish(const DeltaStatePacket& packet);
    void broadcast(const PlayerDisconnectedPacket& packet);
    void broadcast(const EntitySpawnPacket& packet);
    void broadcast(const EntityDestroyedPacket& packet);
    IpEndpoint endpoint() const;

  private:
    void run();

    IpEndpoint bind_;
    std::vector<IpEndpoint> clients_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    UdpSocket socket_;
    double hz_;
    std::mutex payloadMutex_;
    std::vector<std::uint8_t> latest_;
};
