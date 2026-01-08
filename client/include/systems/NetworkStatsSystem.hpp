#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "network/SnapshotParser.hpp"
#include "systems/ISystem.hpp"

#include <chrono>
#include <cstdint>

class NetworkStatsSystem : public ISystem
{
  public:
    NetworkStatsSystem(ThreadSafeQueue<SnapshotParseResult>& snapshots);

    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;

    void recordPacketSent(std::size_t bytes);
    void recordPacketReceived(std::size_t bytes);

  private:
    ThreadSafeQueue<SnapshotParseResult>& snapshots_;

    std::chrono::steady_clock::time_point lastSnapshotTime_;
    std::uint32_t lastSnapshotTick_ = 0;

    std::uint32_t bytesReceivedThisFrame_ = 0;
    std::uint32_t bytesSentThisFrame_     = 0;
};
