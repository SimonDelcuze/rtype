#pragma once

#include <cstdint>
#include <string>

using EntityId = std::uint32_t;

struct EntitySpawnedEvent
{
    EntityId entityId;
    float x;
    float y;
    std::string entityType;
};

struct EntityDestroyedEvent
{
    EntityId entityId;
    std::string entityType;
};

struct EntityDamagedEvent
{
    EntityId entityId;
    EntityId attacker;
    int damageAmount;
    int remainingHealth;
};

struct EntityHealedEvent
{
    EntityId entityId;
    int healAmount;
    int currentHealth;
};

struct PlayerScoredEvent
{
    EntityId playerId;
    int pointsGained;
    int totalScore;
    std::string reason;
};

struct PlayerDiedEvent
{
    EntityId playerId;
    EntityId killer;
    int livesRemaining;
};

struct PlayerRespawnedEvent
{
    EntityId playerId;
    float x;
    float y;
};

struct CollisionEvent
{
    EntityId entityA;
    EntityId entityB;
    float impactX;
    float impactY;
};

struct ProjectileFiredEvent
{
    EntityId projectileId;
    EntityId shooter;
    float x;
    float y;
    float velocityX;
    float velocityY;
};

struct PowerUpCollectedEvent
{
    EntityId playerId;
    EntityId powerUpId;
    std::string powerUpType;
};

struct LevelCompletedEvent
{
    int levelNumber;
    int totalScore;
    float completionTime;
};

struct GameOverEvent
{
    bool victory;
    int finalScore;
    int level;
};

struct WaveStartedEvent
{
    int waveNumber;
    int enemyCount;
};

struct WaveCompletedEvent
{
    int waveNumber;
    int enemiesKilled;
    int bonusPoints;
};

struct BossSpawnedEvent
{
    EntityId bossId;
    std::string bossName;
    int maxHealth;
};

struct BossDefeatedEvent
{
    EntityId bossId;
    std::string bossName;
    int bonusPoints;
};
