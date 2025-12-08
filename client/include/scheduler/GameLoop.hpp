#pragma once

#include "ecs/Registry.hpp"
#include "graphics/Window.hpp"
#include "network/UdpSocket.hpp"
#include "scheduler/ClientScheduler.hpp"

#include <atomic>
#include <cstdint>
#include <memory>

class GameLoop : public ClientScheduler
{
  public:
    GameLoop() = default;

    int run(Window& window, Registry& registry, UdpSocket* networkSocket, const IpEndpoint* serverEndpoint,
            std::atomic<bool>& runningFlag);
};
