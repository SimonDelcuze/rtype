#include "systems/ReconciliationSystem.hpp"

#include "components/VelocityComponent.hpp"

#include <cmath>

void ReconciliationSystem::initialize() {}

void ReconciliationSystem::update(Registry&, float) {}

void ReconciliationSystem::cleanup() {}

void ReconciliationSystem::updateLatencyEstimate(float latencyMs)
{
    reconciliationThreshold_ = baseThreshold_ + (latencyMs * latencyFactor_);
    reconciliationThreshold_ = std::min(reconciliationThreshold_, 5.0F);
}

void ReconciliationSystem::reconcile(Registry& registry, EntityId entityId, float authoritativeX, float authoritativeY,
                                     std::uint32_t acknowledgedSequence)
{
    if (!registry.isAlive(entityId)) {
        return;
    }

    if (!registry.has<TransformComponent>(entityId) || !registry.has<InputHistoryComponent>(entityId)) {
        return;
    }

    auto& transform = registry.get<TransformComponent>(entityId);
    auto& history   = registry.get<InputHistoryComponent>(entityId);

    float predictedX = transform.x;
    float predictedY = transform.y;

    float errorX         = predictedX - authoritativeX;
    float errorY         = predictedY - authoritativeY;
    float errorMagnitude = std::sqrt(errorX * errorX + errorY * errorY);

    float magnitude  = std::sqrt(authoritativeX * authoritativeX + authoritativeY * authoritativeY);
    float percentErr = (magnitude > 0.0F) ? (errorMagnitude / magnitude) : errorMagnitude;

    const bool overHardThreshold = percentErr >= 0.05F;
    const bool overSoftThreshold = percentErr >= 0.02F;

    timeSinceLastReconcile_ += 0.05F;

    if (!overHardThreshold && !(overSoftThreshold && timeSinceLastReconcile_ >= 2.0F)) {
        history.acknowledgeUpTo(acknowledgedSequence);
        return;
    }

    transform.x             = authoritativeX;
    transform.y             = authoritativeY;
    timeSinceLastReconcile_ = 0.0F;

    auto unacknowledgedInputs = history.getInputsAfter(acknowledgedSequence);

    for (const auto& input : unacknowledgedInputs) {
        simulateInput(transform, input, playerMoveSpeed_);
    }

    history.acknowledgeUpTo(acknowledgedSequence);
}

bool ReconciliationSystem::simulateInput(TransformComponent& transform, const InputHistoryEntry& input, float moveSpeed)
{
    float oldX = transform.x;
    float oldY = transform.y;

    applyMovement(transform, input.flags, input.deltaTime, moveSpeed);

    return (transform.x != oldX) || (transform.y != oldY);
}

void ReconciliationSystem::applyMovement(TransformComponent& transform, std::uint16_t flags, float deltaTime,
                                         float moveSpeed)
{
    float dx = 0.0F;
    float dy = 0.0F;

    if ((flags & static_cast<std::uint16_t>(InputFlag::MoveUp)) != 0) {
        dy -= 1.0F;
    }
    if ((flags & static_cast<std::uint16_t>(InputFlag::MoveDown)) != 0) {
        dy += 1.0F;
    }
    if ((flags & static_cast<std::uint16_t>(InputFlag::MoveLeft)) != 0) {
        dx -= 1.0F;
    }
    if ((flags & static_cast<std::uint16_t>(InputFlag::MoveRight)) != 0) {
        dx += 1.0F;
    }

    if (dx != 0.0F || dy != 0.0F) {
        float length = std::sqrt(dx * dx + dy * dy);
        dx /= length;
        dy /= length;

        transform.x += dx * moveSpeed * deltaTime;
        transform.y += dy * moveSpeed * deltaTime;
    }
}
