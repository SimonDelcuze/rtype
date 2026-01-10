#pragma once

#include "ecs/Registry.hpp"
#include "levels/LevelData.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct DispatchedEvent
{
    LevelEvent event;
    std::string segmentId;
    std::string bossId;
    bool fromBoss = false;
};

class LevelDirector
{
  public:
    explicit LevelDirector(LevelData data);

    void reset();
    void update(Registry& registry, float deltaTime);

    std::vector<DispatchedEvent> consumeEvents();

    void registerSpawn(const std::string& spawnId, EntityId entityId);
    void unregisterSpawn(const std::string& spawnId);
    void registerBoss(const std::string& bossId, EntityId entityId);
    void unregisterBoss(const std::string& bossId);
    void registerPlayerInput(EntityId playerId, std::uint16_t flags);

    const LevelSegment* currentSegment() const;
    std::int32_t currentSegmentIndex() const;
    float segmentTime() const;
    float segmentDistance() const;
    bool finished() const;
    const std::optional<CameraBounds>& playerBounds() const;

  private:
    struct EventRuntime
    {
        const LevelEvent* event = nullptr;
        bool fired              = false;
        bool repeating          = false;
        float nextRepeatTime    = 0.0F;
        std::optional<std::int32_t> remainingCount;
    };

    struct BossRuntime
    {
        EntityId entityId        = 0;
        bool registered          = false;
        bool dead                = false;
        bool onDeathFired        = false;
        std::size_t phaseIndex   = 0;
        float phaseStartTime     = 0.0F;
        float phaseStartDistance = 0.0F;
        std::vector<EventRuntime> phaseEvents;
    };

    struct TriggerContext
    {
        float time              = 0.0F;
        float distance          = 0.0F;
        std::int32_t enemyCount = 0;
        Registry* registry      = nullptr;
    };

    void enterSegment(std::size_t index);
    void updateSegmentEvents(Registry& registry);
    void updateBossEvents(Registry& registry);
    bool evaluateExit(Registry& registry) const;

    bool isTriggerActive(const Trigger& trigger, const TriggerContext& ctx) const;
    bool isSpawnDead(const std::string& spawnId, Registry& registry) const;
    bool isBossDead(const std::string& bossId, Registry& registry) const;
    bool isBossHpBelow(const std::string& bossId, std::int32_t value, Registry& registry) const;
    bool isPlayerInZone(const Trigger& trigger, Registry& registry) const;
    bool arePlayersReady(Registry& registry) const;
    std::int32_t countEnemies(Registry& registry) const;

    void fireEvent(const LevelEvent& event, const std::string& segmentId, const std::string& bossId, bool fromBoss);
    void applyEventEffects(const LevelEvent& event);
    void setupRepeat(EventRuntime& runtime, float now);
    bool processRepeat(EventRuntime& runtime, float now, const TriggerContext& ctx);
    std::vector<EventRuntime> makeEventRuntime(const std::vector<LevelEvent>& events) const;

    float currentScrollSpeed() const;

    LevelData data_;
    std::size_t segmentIndex_ = 0;
    float segmentTime_        = 0.0F;
    float segmentDistance_    = 0.0F;
    ScrollSettings activeScroll_;
    std::optional<CameraBounds> activePlayerBounds_;
    std::vector<EventRuntime> segmentEvents_;
    std::vector<DispatchedEvent> firedEvents_;

    struct SpawnGroup
    {
        std::unordered_set<EntityId> entities;
        bool spawned = false;
    };

    std::unordered_map<std::string, SpawnGroup> spawnEntities_;
    std::unordered_map<std::string, BossRuntime> bossStates_;
    std::unordered_set<std::string> checkpoints_;
    std::unordered_set<EntityId> readyPlayers_;
    std::unordered_map<EntityId, bool> readyInputHeld_;
    bool finished_ = false;
};
