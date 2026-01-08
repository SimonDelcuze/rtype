#include "levels/LevelDirector.hpp"

#include "components/HealthComponent.hpp"
#include "components/RespawnTimerComponent.hpp"
#include "components/TagComponent.hpp"
#include "components/TransformComponent.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

LevelDirector::LevelDirector(LevelData data) : data_(std::move(data))
{
    reset();
}

void LevelDirector::reset()
{
    segmentIndex_    = 0;
    segmentTime_     = 0.0F;
    segmentDistance_ = 0.0F;
    firedEvents_.clear();
    spawnEntities_.clear();
    bossStates_.clear();
    checkpoints_.clear();
    activePlayerBounds_.reset();
    finished_ = data_.segments.empty();
    if (!finished_) {
        enterSegment(0);
    }
}

LevelDirector::CheckpointState LevelDirector::captureCheckpointState() const
{
    CheckpointState state;
    state.segmentIndex    = segmentIndex_;
    state.segmentTime     = segmentTime_;
    state.segmentDistance = segmentDistance_;
    state.activeScroll    = activeScroll_;
    state.playerBounds    = activePlayerBounds_;
    state.finished        = finished_;
    state.checkpoints     = checkpoints_;

    state.segmentEvents.reserve(segmentEvents_.size());
    for (const auto& runtime : segmentEvents_) {
        EventRuntimeState evState;
        evState.fired          = runtime.fired;
        evState.repeating      = runtime.repeating;
        evState.nextRepeatTime = runtime.nextRepeatTime;
        evState.remainingCount = runtime.remainingCount;
        state.segmentEvents.push_back(std::move(evState));
    }

    state.spawnGroups.reserve(spawnEntities_.size());
    for (const auto& [spawnId, group] : spawnEntities_) {
        SpawnGroupState spawnState;
        spawnState.spawnId = spawnId;
        spawnState.spawned = group.spawned;
        state.spawnGroups.push_back(std::move(spawnState));
    }

    state.bosses.reserve(bossStates_.size());
    for (const auto& [bossId, runtime] : bossStates_) {
        if (!runtime.registered)
            continue;
        BossCheckpointState bossState;
        bossState.bossId = bossId;
        bossState.status = runtime.dead ? BossCheckpointStatus::Dead : BossCheckpointStatus::Alive;
        state.bosses.push_back(std::move(bossState));
    }

    return state;
}

void LevelDirector::restoreCheckpointState(const CheckpointState& state)
{
    firedEvents_.clear();
    spawnEntities_.clear();
    bossStates_.clear();

    if (data_.segments.empty()) {
        finished_ = true;
        return;
    }
    if (state.segmentIndex >= data_.segments.size()) {
        reset();
        return;
    }

    segmentIndex_    = state.segmentIndex;
    segmentTime_     = state.segmentTime;
    segmentDistance_ = state.segmentDistance;
    activeScroll_    = state.activeScroll;
    activePlayerBounds_ = state.playerBounds;
    segmentEvents_   = makeEventRuntime(data_.segments[segmentIndex_].events);

    const std::size_t count = std::min(segmentEvents_.size(), state.segmentEvents.size());
    for (std::size_t i = 0; i < count; ++i) {
        auto& runtime          = segmentEvents_[i];
        const auto& snapshot   = state.segmentEvents[i];
        runtime.fired          = snapshot.fired;
        runtime.repeating      = snapshot.repeating;
        runtime.nextRepeatTime = snapshot.nextRepeatTime;
        runtime.remainingCount = snapshot.remainingCount;
    }

    for (const auto& spawnState : state.spawnGroups) {
        SpawnGroup group;
        group.spawned = spawnState.spawned;
        spawnEntities_.emplace(spawnState.spawnId, std::move(group));
    }

    for (const auto& bossState : state.bosses) {
        if (bossState.status != BossCheckpointStatus::Dead)
            continue;
        BossRuntime runtime;
        runtime.entityId     = std::numeric_limits<EntityId>::max();
        runtime.registered   = true;
        runtime.dead         = true;
        runtime.onDeathFired = true;
        bossStates_.emplace(bossState.bossId, std::move(runtime));
    }

    checkpoints_ = state.checkpoints;
    finished_    = state.finished;
}

void LevelDirector::enterSegment(std::size_t index)
{
    segmentIndex_    = index;
    segmentTime_     = 0.0F;
    segmentDistance_ = 0.0F;
    activeScroll_    = data_.segments[index].scroll;
    activePlayerBounds_.reset();
    segmentEvents_   = makeEventRuntime(data_.segments[index].events);
}

void LevelDirector::registerSpawn(const std::string& spawnId, EntityId entityId)
{
    if (!spawnId.empty()) {
        auto& group   = spawnEntities_[spawnId];
        group.spawned = true;
        group.entities.insert(entityId);
    }
}

void LevelDirector::unregisterSpawn(const std::string& spawnId)
{
    spawnEntities_.erase(spawnId);
}

void LevelDirector::registerBoss(const std::string& bossId, EntityId entityId)
{
    if (bossId.empty())
        return;
    auto& state        = bossStates_[bossId];
    state.entityId     = entityId;
    state.registered   = true;
    state.dead         = false;
    state.onDeathFired = false;
    state.phaseIndex   = 0;
    state.phaseEvents.clear();
    state.phaseStartTime     = segmentTime_;
    state.phaseStartDistance = segmentDistance_;
}

void LevelDirector::unregisterBoss(const std::string& bossId)
{
    bossStates_.erase(bossId);
}

void LevelDirector::markCheckpointReached(const std::string& checkpointId)
{
    if (!checkpointId.empty())
        checkpoints_.insert(checkpointId);
}

const LevelSegment* LevelDirector::currentSegment() const
{
    if (finished_ || segmentIndex_ >= data_.segments.size())
        return nullptr;
    return &data_.segments[segmentIndex_];
}

std::int32_t LevelDirector::currentSegmentIndex() const
{
    if (finished_)
        return -1;
    return static_cast<std::int32_t>(segmentIndex_);
}

float LevelDirector::segmentTime() const
{
    return segmentTime_;
}

float LevelDirector::segmentDistance() const
{
    return segmentDistance_;
}

bool LevelDirector::finished() const
{
    return finished_;
}

const std::optional<CameraBounds>& LevelDirector::playerBounds() const
{
    return activePlayerBounds_;
}

float LevelDirector::currentScrollSpeed() const
{
    if (activeScroll_.mode == ScrollMode::Stopped)
        return 0.0F;
    if (activeScroll_.mode == ScrollMode::Constant)
        return activeScroll_.speedX;
    if (activeScroll_.curve.empty())
        return 0.0F;
    float speed = activeScroll_.curve.front().speedX;
    for (const auto& key : activeScroll_.curve) {
        if (segmentTime_ >= key.time)
            speed = key.speedX;
        else
            break;
    }
    return speed;
}

void LevelDirector::update(Registry& registry, float deltaTime)
{
    if (finished_ || data_.segments.empty())
        return;

    float speed = currentScrollSpeed();
    segmentTime_ += deltaTime;
    segmentDistance_ += std::abs(speed) * deltaTime;

    std::size_t transitions = 0;
    while (!finished_) {
        updateSegmentEvents(registry);
        updateBossEvents(registry);
        if (!evaluateExit(registry))
            break;
        transitions++;
        if (segmentIndex_ + 1 >= data_.segments.size()) {
            finished_ = true;
            break;
        }
        enterSegment(segmentIndex_ + 1);
        if (transitions >= data_.segments.size())
            break;
    }
}

std::vector<DispatchedEvent> LevelDirector::consumeEvents()
{
    std::vector<DispatchedEvent> out = std::move(firedEvents_);
    firedEvents_.clear();
    return out;
}

bool LevelDirector::isSpawnDead(const std::string& spawnId, Registry& registry) const
{
    auto it = spawnEntities_.find(spawnId);
    if (it == spawnEntities_.end())
        return false;
    if (!it->second.spawned)
        return false;
    const auto& group = it->second;
    for (const auto& entity : group.entities) {
        if (registry.isAlive(entity))
            return false;
    }
    return true;
}

bool LevelDirector::isBossDead(const std::string& bossId, Registry& registry) const
{
    auto it = bossStates_.find(bossId);
    if (it == bossStates_.end())
        return false;
    if (!it->second.registered)
        return false;
    return !registry.isAlive(it->second.entityId);
}

bool LevelDirector::isBossHpBelow(const std::string& bossId, std::int32_t value, Registry& registry) const
{
    auto it = bossStates_.find(bossId);
    if (it == bossStates_.end())
        return false;
    if (!it->second.registered)
        return false;
    if (!registry.isAlive(it->second.entityId))
        return false;
    if (!registry.has<HealthComponent>(it->second.entityId))
        return false;
    const auto& h = registry.get<HealthComponent>(it->second.entityId);
    return h.current <= value;
}

std::int32_t LevelDirector::countEnemies(Registry& registry) const
{
    std::int32_t count = 0;
    for (EntityId id : registry.view<TagComponent>()) {
        if (!registry.isAlive(id))
            continue;
        const auto& tag = registry.get<TagComponent>(id);
        if (tag.hasTag(EntityTag::Enemy))
            count++;
    }
    return count;
}

bool LevelDirector::isPlayerInZone(const Trigger& trigger, Registry& registry) const
{
    if (!trigger.zone.has_value())
        return false;
    const auto& bounds = *trigger.zone;
    std::int32_t players = 0;
    std::int32_t inside  = 0;
    for (EntityId id : registry.view<TransformComponent, TagComponent>()) {
        if (!registry.isAlive(id))
            continue;
        const auto& tag = registry.get<TagComponent>(id);
        if (!tag.hasTag(EntityTag::Player))
            continue;
        if (registry.has<RespawnTimerComponent>(id))
            continue;
        players++;
        const auto& t = registry.get<TransformComponent>(id);
        if (t.x >= bounds.minX && t.x <= bounds.maxX && t.y >= bounds.minY && t.y <= bounds.maxY)
            inside++;
    }
    if (players <= 0)
        return false;
    if (trigger.requireAllPlayers)
        return inside == players;
    return inside > 0;
}

bool LevelDirector::isTriggerActive(const Trigger& trigger, const TriggerContext& ctx) const
{
    switch (trigger.type) {
        case TriggerType::Time:
            return ctx.time >= trigger.time;
        case TriggerType::Distance:
            return ctx.distance >= trigger.distance;
        case TriggerType::SpawnDead:
            return isSpawnDead(trigger.spawnId, *ctx.registry);
        case TriggerType::BossDead:
            return isBossDead(trigger.bossId, *ctx.registry);
        case TriggerType::EnemyCountAtMost:
            return ctx.enemyCount <= trigger.count;
        case TriggerType::CheckpointReached:
            return checkpoints_.find(trigger.checkpointId) != checkpoints_.end();
        case TriggerType::HpBelow:
            return isBossHpBelow(trigger.bossId, trigger.value, *ctx.registry);
        case TriggerType::AllOf:
            for (const auto& child : trigger.triggers) {
                if (!isTriggerActive(child, ctx))
                    return false;
            }
            return true;
        case TriggerType::AnyOf:
            for (const auto& child : trigger.triggers) {
                if (isTriggerActive(child, ctx))
                    return true;
            }
            return false;
        case TriggerType::PlayerInZone:
            return isPlayerInZone(trigger, *ctx.registry);
        default:
            return false;
    }
}

void LevelDirector::fireEvent(const LevelEvent& event, const std::string& segmentId, const std::string& bossId,
                              bool fromBoss)
{
    firedEvents_.push_back(DispatchedEvent{event, segmentId, bossId, fromBoss});
    applyEventEffects(event);
}

void LevelDirector::applyEventEffects(const LevelEvent& event)
{
    if (event.type == EventType::SetScroll && event.scroll) {
        activeScroll_ = *event.scroll;
    } else if (event.type == EventType::Checkpoint && event.checkpoint) {
        checkpoints_.insert(event.checkpoint->checkpointId);
    } else if (event.type == EventType::SetPlayerBounds && event.playerBounds) {
        activePlayerBounds_ = *event.playerBounds;
    } else if (event.type == EventType::ClearPlayerBounds) {
        activePlayerBounds_.reset();
    }
}

void LevelDirector::setupRepeat(EventRuntime& runtime, float now)
{
    runtime.repeating      = true;
    runtime.nextRepeatTime = now + runtime.event->repeat->interval;
    if (runtime.event->repeat->count.has_value()) {
        runtime.remainingCount = *runtime.event->repeat->count - 1;
        if (*runtime.remainingCount <= 0) {
            runtime.repeating = false;
        }
    } else {
        runtime.remainingCount.reset();
    }
}

bool LevelDirector::processRepeat(EventRuntime& runtime, float now, const TriggerContext& ctx)
{
    if (!runtime.repeating)
        return false;
    if (runtime.event->repeat->until.has_value()) {
        if (isTriggerActive(*runtime.event->repeat->until, ctx)) {
            runtime.repeating = false;
            return false;
        }
    }
    if (now < runtime.nextRepeatTime)
        return false;
    if (runtime.remainingCount.has_value()) {
        if (*runtime.remainingCount <= 0) {
            runtime.repeating = false;
            return false;
        }
        (*runtime.remainingCount)--;
        if (*runtime.remainingCount <= 0)
            runtime.repeating = false;
    }
    runtime.nextRepeatTime = now + runtime.event->repeat->interval;
    return true;
}

void LevelDirector::updateSegmentEvents(Registry& registry)
{
    if (segmentIndex_ >= data_.segments.size())
        return;
    const auto& segment = data_.segments[segmentIndex_];
    TriggerContext ctx;
    ctx.time       = segmentTime_;
    ctx.distance   = segmentDistance_;
    ctx.registry   = &registry;
    ctx.enemyCount = countEnemies(registry);

    for (auto& runtime : segmentEvents_) {
        if (runtime.event == nullptr)
            continue;
        if (runtime.fired && !runtime.repeating)
            continue;
        if (runtime.fired && runtime.repeating) {
            if (processRepeat(runtime, ctx.time, ctx)) {
                fireEvent(*runtime.event, segment.id, "", false);
            }
            continue;
        }
        if (isTriggerActive(runtime.event->trigger, ctx)) {
            runtime.fired = true;
            fireEvent(*runtime.event, segment.id, "", false);
            if (runtime.event->repeat.has_value()) {
                setupRepeat(runtime, ctx.time);
            }
        }
    }
}

void LevelDirector::updateBossEvents(Registry& registry)
{
    for (auto& [bossId, state] : bossStates_) {
        if (!state.registered)
            continue;
        bool alive = registry.isAlive(state.entityId);
        if (!alive && !state.onDeathFired) {
            state.dead         = true;
            state.onDeathFired = true;
            auto it            = data_.bosses.find(bossId);
            if (it != data_.bosses.end()) {
                for (const auto& ev : it->second.onDeath) {
                    fireEvent(ev, data_.segments[segmentIndex_].id, bossId, true);
                }
            }
            continue;
        }
        if (!alive)
            continue;
        auto defIt = data_.bosses.find(bossId);
        if (defIt == data_.bosses.end())
            continue;
        const auto& def = defIt->second;

        if (state.phaseIndex < def.phases.size()) {
            const auto& phase = def.phases[state.phaseIndex];
            TriggerContext phaseCtx;
            phaseCtx.time       = segmentTime_;
            phaseCtx.distance   = segmentDistance_;
            phaseCtx.registry   = &registry;
            phaseCtx.enemyCount = countEnemies(registry);
            if (isTriggerActive(phase.trigger, phaseCtx)) {
                state.phaseStartTime     = segmentTime_;
                state.phaseStartDistance = segmentDistance_;
                state.phaseEvents        = makeEventRuntime(phase.events);
                state.phaseIndex++;
            }
        }

        if (!state.phaseEvents.empty()) {
            TriggerContext phaseCtx;
            phaseCtx.time       = segmentTime_ - state.phaseStartTime;
            phaseCtx.distance   = segmentDistance_ - state.phaseStartDistance;
            phaseCtx.registry   = &registry;
            phaseCtx.enemyCount = countEnemies(registry);

            for (auto& runtime : state.phaseEvents) {
                if (runtime.event == nullptr)
                    continue;
                if (runtime.fired && !runtime.repeating)
                    continue;
                if (runtime.fired && runtime.repeating) {
                    if (processRepeat(runtime, phaseCtx.time, phaseCtx)) {
                        fireEvent(*runtime.event, data_.segments[segmentIndex_].id, bossId, true);
                    }
                    continue;
                }
                if (isTriggerActive(runtime.event->trigger, phaseCtx)) {
                    runtime.fired = true;
                    fireEvent(*runtime.event, data_.segments[segmentIndex_].id, bossId, true);
                    if (runtime.event->repeat.has_value()) {
                        setupRepeat(runtime, phaseCtx.time);
                    }
                }
            }
        }
    }
}

bool LevelDirector::evaluateExit(Registry& registry) const
{
    if (segmentIndex_ >= data_.segments.size())
        return false;
    TriggerContext ctx;
    ctx.time       = segmentTime_;
    ctx.distance   = segmentDistance_;
    ctx.registry   = &registry;
    ctx.enemyCount = countEnemies(registry);
    return isTriggerActive(data_.segments[segmentIndex_].exit, ctx);
}

std::vector<LevelDirector::EventRuntime> LevelDirector::makeEventRuntime(const std::vector<LevelEvent>& events) const
{
    std::vector<EventRuntime> out;
    out.reserve(events.size());
    for (const auto& ev : events) {
        EventRuntime runtime;
        runtime.event = &ev;
        out.push_back(runtime);
    }
    return out;
}
