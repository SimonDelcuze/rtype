#pragma once

#include "ecs/Registry.hpp"
#include "systems/ISystem.hpp"

#include <atomic>
#include <cstdint>

class NetworkStatsSystem : public ISystem
{
  public:
    NetworkStatsSystem() = default;

    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;

    void recordPacketSent(std::size_t bytes);
    void recordPacketReceived(std::size_t bytes);

  private:
    std::uint32_t bytesReceivedThisFrame_ = 0;
    std::uint32_t bytesSentThisFrame_     = 0;
};

extern std::atomic<int64_t> g_lastPingTimeMicros;
extern std::atomic<bool> g_pingPending;
extern std::atomic<float> g_lastRtt;
extern std::atomic<bool> g_rttConsumed;

void recordGlobalPingSent();
float recordGlobalPongReceived();
