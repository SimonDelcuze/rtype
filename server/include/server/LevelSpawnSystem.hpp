#pragma once

#include "ecs/Registry.hpp"
#include "server/LevelData.hpp"
#include "server/LevelDirector.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class LevelSpawnSystem
{
  public:
    LevelSpawnSystem(const LevelData& data, LevelDirector* director, float playfieldHeight = 720.0F);

    void reset();
    void update(Registry& registry, float deltaTime, const std::vector<DispatchedEvent>& events);

  private:
    struct PendingEnemySpawn
    {
        float time = 0.0F;
        MovementComponent movement;
        HitboxComponent hitbox;
        ColliderComponent collider;
        std::int32_t health = 1;
        Vec2f scale{1.0F, 1.0F};
        std::optional<EnemyShootingComponent> shooting;
        std::uint16_t typeId = 0;
        float x = 0.0F;
        float y = 0.0F;
        std::string spawnGroupId;
    };

    void dispatchEvents(Registry& registry, const std::vector<DispatchedEvent>& events);
    void spawnPending(Registry& registry);

    void scheduleWave(const LevelEvent& event, const WaveDefinition& wave);
    void enqueueEnemySpawn(float timeOffset, const EnemyTemplate& enemy, const MovementComponent& movement, float x,
                           float y, const WaveDefinition& wave, const std::string& spawnGroupId);

    void spawnEnemy(Registry& registry, const PendingEnemySpawn& spawn);
    void spawnObstacle(Registry& registry, const SpawnObstacleSettings& settings, const LevelEvent& event);
    void spawnBoss(Registry& registry, const SpawnBossSettings& settings);

    float resolveObstacleY(const ObstacleTemplate& tpl, const SpawnObstacleSettings& settings, float scaleY) const;

    const LevelData* data_ = nullptr;
    LevelDirector* director_ = nullptr;
    float playfieldHeight_ = 720.0F;
    float time_ = 0.0F;

    std::unordered_map<std::string, MovementComponent> patternMap_;
    std::vector<PendingEnemySpawn> pendingEnemies_;
};
