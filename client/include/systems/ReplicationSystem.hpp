#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "network/SnapshotParser.hpp"
#include "systems/ISystem.hpp"

#include <unordered_map>

class ReplicationSystem : public ISystem
{
  public:
    explicit ReplicationSystem(ThreadSafeQueue<SnapshotParseResult>& snapshots);

    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;

  private:
    EntityId ensureEntity(Registry& registry, std::uint32_t remoteId);
    void applyEntity(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyTransform(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyVelocity(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyHealth(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyStatus(Registry&, EntityId, const SnapshotEntity&);
    void applyDead(Registry& registry, EntityId id, const SnapshotEntity& entity);
    void applyInterpolation(Registry& registry, EntityId id, const SnapshotEntity& entity, std::uint32_t tickId);

    ThreadSafeQueue<SnapshotParseResult>* snapshots_;
    std::unordered_map<std::uint32_t, EntityId> remoteToLocal_;
};
