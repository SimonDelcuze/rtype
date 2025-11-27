#pragma once

#include "components/InputHistoryComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "network/InputPacket.hpp"
#include "systems/ISystem.hpp"

#include <cstdint>

class ReconciliationSystem : public ISystem
{
  public:
    ReconciliationSystem() = default;

    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;
    void reconcile(Registry& registry, EntityId entityId, float authoritativeX, float authoritativeY,
                   std::uint32_t acknowledgedSequence);

  private:
    bool simulateInput(TransformComponent& transform, const InputHistoryEntry& input, float moveSpeed);
    void applyMovement(TransformComponent& transform, std::uint16_t flags, float deltaTime, float moveSpeed);

    float playerMoveSpeed_         = 200.0F;
    float reconciliationThreshold_ = 0.5F;
};
