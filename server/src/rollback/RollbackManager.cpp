#include "rollback/RollbackManager.hpp"

#include "components/HealthComponent.hpp"
#include "components/InvincibilityComponent.hpp"
#include "components/LivesComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"
#include "ecs/Registry.hpp"
#include "replication/EntityStateCache.hpp"
#include "core/EntityTypeResolver.hpp"

#include <algorithm>

RollbackManager::RollbackManager() = default;

std::unordered_map<EntityId, CachedEntityState> RollbackManager::extractEntityStates(
    const Registry& registry) const
{
    std::unordered_map<EntityId, CachedEntityState> states;

    for (EntityId id = 0; id < registry.nextId_; id++)
    {
        if (!registry.isAlive(id) || !registry.has<TransformComponent>(id))
        {
            continue;
        }

        CachedEntityState state{};
        const auto& transform = registry.get<TransformComponent>(id);
        state.posX            = transform.x;
        state.posY            = transform.y;
        state.entityType      = resolveEntityType(registry, id);

        if (registry.has<VelocityComponent>(id))
        {
            const auto& velocity = registry.get<VelocityComponent>(id);
            state.velX           = velocity.vx;
            state.velY           = velocity.vy;
        }

        if (registry.has<HealthComponent>(id))
        {
            state.health = static_cast<std::int16_t>(registry.get<HealthComponent>(id).current);
        }

        if (registry.has<LivesComponent>(id))
        {
            state.lives = static_cast<std::int8_t>(registry.get<LivesComponent>(id).current);
        }

        if (registry.has<ScoreComponent>(id))
        {
            state.score = registry.get<ScoreComponent>(id).value;
        }

        state.status = 0;
        if (registry.has<InvincibilityComponent>(id))
        {
            state.status |= (1 << 1);
        }

        state.initialized = true;
        states[id]        = state;
    }

    return states;
}

std::uint32_t RollbackManager::captureState(std::uint64_t tick, const Registry& registry)
{
    std::lock_guard<std::mutex> lock(historyMutex_);

    auto states = extractEntityStates(registry);

    std::uint32_t checksum = StateChecksum::compute(states);

    stateHistory_.addSnapshot(tick, states, checksum);

    return checksum;
}

std::optional<std::reference_wrapper<const StateSnapshot>> RollbackManager::getSnapshot(
    std::uint64_t tick) const
{
    std::lock_guard<std::mutex> lock(historyMutex_);
    return stateHistory_.getSnapshot(tick);
}

bool RollbackManager::canRollbackTo(std::uint64_t tick) const
{
    std::lock_guard<std::mutex> lock(historyMutex_);
    return stateHistory_.hasSnapshot(tick);
}

std::optional<std::uint32_t> RollbackManager::getChecksum(std::uint64_t tick) const
{
    std::lock_guard<std::mutex> lock(historyMutex_);
    auto snapshot = stateHistory_.getSnapshot(tick);
    if (snapshot)
    {
        return snapshot->get().checksum;
    }
    return std::nullopt;
}

std::optional<std::pair<std::uint64_t, std::uint64_t>> RollbackManager::getTickRange() const
{
    std::lock_guard<std::mutex> lock(historyMutex_);
    return stateHistory_.getTickRange();
}

void RollbackManager::restoreEntityStates(
    Registry& registry, const std::unordered_map<EntityId, CachedEntityState>& states) const
{
    std::vector<EntityId> entitiesToDestroy;
    for (EntityId id = 0; id < registry.nextId_; id++)
    {
        if (registry.isAlive(id) && registry.has<TransformComponent>(id) &&
            states.find(id) == states.end())
        {
            entitiesToDestroy.push_back(id);
        }
    }
    for (EntityId id : entitiesToDestroy)
    {
        registry.destroyEntity(id);
    }

    for (const auto& [id, state] : states)
    {
        if (!state.initialized)
        {
            continue;
        }

        while (id >= registry.nextId_)
        {
            registry.createEntity();
        }

        if (!registry.isAlive(id))
        {
            continue;
        }

        if (!registry.has<TransformComponent>(id))
        {
            registry.emplace<TransformComponent>(id);
        }
        auto& transform = registry.get<TransformComponent>(id);
        transform.x     = state.posX;
        transform.y     = state.posY;

        if (state.velX != 0.0F || state.velY != 0.0F)
        {
            if (!registry.has<VelocityComponent>(id))
            {
                registry.emplace<VelocityComponent>(id);
            }
            auto& velocity = registry.get<VelocityComponent>(id);
            velocity.vx    = state.velX;
            velocity.vy    = state.velY;
        }

        if (registry.has<HealthComponent>(id))
        {
            auto& health   = registry.get<HealthComponent>(id);
            health.current = state.health;
        }

        if (registry.has<LivesComponent>(id))
        {
            auto& lives   = registry.get<LivesComponent>(id);
            lives.current = state.lives;
        }

        if (registry.has<ScoreComponent>(id))
        {
            auto& score = registry.get<ScoreComponent>(id);
            score.value = state.score;
        }

        bool hasInvincibility = (state.status & (1 << 1)) != 0;
        if (hasInvincibility && !registry.has<InvincibilityComponent>(id))
        {
            registry.emplace<InvincibilityComponent>(id);
        }
        else if (!hasInvincibility && registry.has<InvincibilityComponent>(id))
        {
            registry.remove<InvincibilityComponent>(id);
        }
    }
}

bool RollbackManager::rollbackTo(std::uint64_t tick, Registry& registry)
{
    std::lock_guard<std::mutex> lock(historyMutex_);

    auto snapshot = stateHistory_.getSnapshot(tick);
    if (!snapshot)
    {
        return false;
    }

    restoreEntityStates(registry, snapshot->get().entities);
    return true;
}

void RollbackManager::clear()
{
    std::lock_guard<std::mutex> lock(historyMutex_);
    stateHistory_.clear();
}

std::size_t RollbackManager::getHistorySize() const
{
    std::lock_guard<std::mutex> lock(historyMutex_);
    return stateHistory_.size();
}
