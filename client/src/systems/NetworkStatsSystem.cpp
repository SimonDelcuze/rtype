#include "systems/NetworkStatsSystem.hpp"

#include "components/NetworkStatsComponent.hpp"

NetworkStatsSystem::NetworkStatsSystem(ThreadSafeQueue<SnapshotParseResult>& snapshots)
    : snapshots_(snapshots), lastSnapshotTime_(std::chrono::steady_clock::now())
{}

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

    SnapshotParseResult snapshot;
    bool receivedSnapshot = false;
    std::uint32_t latestTick = lastSnapshotTick_;

    while (snapshots_.tryPop(snapshot)) {
        receivedSnapshot = true;
        std::uint32_t estimatedSize = sizeof(PacketHeader) + snapshot.entities.size() * sizeof(SnapshotEntity);
        bytesReceivedThisFrame_ += estimatedSize;

        auto now = std::chrono::steady_clock::now();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSnapshotTime_).count();

        if (snapshot.header.tickId > lastSnapshotTick_) {
            float tickDiff = static_cast<float>(snapshot.header.tickId - lastSnapshotTick_);
            float expectedTime = tickDiff * (1000.0F / 60.0F);
            float actualTime = static_cast<float>(elapsed);

            float ping = std::max(0.0F, actualTime - expectedTime);
            stats.addPingSample(ping);

            lastSnapshotTick_ = snapshot.header.tickId;
            lastSnapshotTime_ = now;
            latestTick = snapshot.header.tickId;
        }

        stats.packetsReceived++;
    }

    stats.addBandwidthSample(bytesReceivedThisFrame_, bytesSentThisFrame_);

    bytesReceivedThisFrame_ = 0;
    bytesSentThisFrame_     = 0;

    if (receivedSnapshot) {
        stats.packetsSent = latestTick;
        stats.updatePacketLoss();
    }
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
