#include "systems/NetworkStatsSystem.hpp"

#include "components/NetworkStatsComponent.hpp"

#include <chrono>

void NetworkStatsSystem::initialize() {}

void NetworkStatsSystem::update(Registry& registry, float deltaTime)
{
    EntityId statsEntity = 0;
    bool found           = false;

    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (registry.isAlive(id) && registry.has<NetworkStatsComponent>(id)) {
            statsEntity = id;
            found       = true;
            break;
        }
    }

    if (!found) {
        statsEntity = registry.createEntity();
        registry.emplace<NetworkStatsComponent>(statsEntity, NetworkStatsComponent::create());
    }

    auto& stats = registry.get<NetworkStatsComponent>(statsEntity);
    stats.timeSinceUpdate += deltaTime;

    if (!g_rttConsumed.load()) {
        float lastRtt = g_lastRtt.load();
        if (lastRtt > 0.0F) {
            stats.addPingSample(lastRtt);
            g_rttConsumed.store(true);
        }
    }

    stats.addBandwidthSample(bytesReceivedThisFrame_, bytesSentThisFrame_);

    bytesReceivedThisFrame_ = 0;
    bytesSentThisFrame_     = 0;
}

void NetworkStatsSystem::cleanup() {}

void NetworkStatsSystem::recordPacketSent(std::size_t bytes)
{
    bytesSentThisFrame_ += static_cast<std::uint32_t>(bytes);
}

void NetworkStatsSystem::recordPacketReceived(std::size_t bytes)
{
    bytesReceivedThisFrame_ += static_cast<std::uint32_t>(bytes);
}

std::atomic<int64_t> g_lastPingTimeMicros{0};
std::atomic<bool> g_pingPending{false};
std::atomic<float> g_lastRtt{0.0F};
std::atomic<bool> g_rttConsumed{true};

void recordGlobalPingSent()
{
    auto now = std::chrono::steady_clock::now();
    g_lastPingTimeMicros.store(std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
    g_pingPending.store(true);
}

float recordGlobalPongReceived()
{
    if (!g_pingPending.load()) {
        return 0.0F;
    }

    auto now           = std::chrono::steady_clock::now();
    int64_t nowMicros  = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    int64_t sentMicros = g_lastPingTimeMicros.load();
    float rtt          = static_cast<float>(nowMicros - sentMicros) / 1000.0F;

    g_pingPending.store(false);
    g_lastRtt.store(rtt);
    g_rttConsumed.store(false);

    return rtt;
}
