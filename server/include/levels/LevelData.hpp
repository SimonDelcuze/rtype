#pragma once

#include "components/Components.hpp"
#include "network/Packets.hpp"
#include "systems/ObstacleSpawnSystem.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct Vec2f
{
    float x = 0.0F;
    float y = 0.0F;
};

struct LevelMeta
{
    std::string name;
    std::string backgroundId;
    std::string musicId;
    std::string author;
    std::string difficulty;
};

struct PatternDefinition
{
    std::string id;
    MovementComponent movement;
};

struct EnemyTemplate
{
    std::uint16_t typeId = 0;
    HitboxComponent hitbox;
    ColliderComponent collider;
    std::int32_t health = 1;
    std::int32_t score  = 0;
    Vec2f scale{1.0F, 1.0F};
    std::optional<EnemyShootingComponent> shooting;
};

struct ObstacleTemplate
{
    std::uint16_t typeId = 0;
    HitboxComponent hitbox;
    ColliderComponent collider;
    std::int32_t health   = 1;
    ObstacleAnchor anchor = ObstacleAnchor::Absolute;
    float margin          = 0.0F;
    float speedX          = 0.0F;
    float speedY          = 0.0F;
    Vec2f scale{1.0F, 1.0F};
};

struct LevelTemplates
{
    std::unordered_map<std::string, HitboxComponent> hitboxes;
    std::unordered_map<std::string, ColliderComponent> colliders;
    std::unordered_map<std::string, EnemyTemplate> enemies;
    std::unordered_map<std::string, ObstacleTemplate> obstacles;
};

enum class ScrollMode : std::uint8_t
{
    Constant,
    Stopped,
    Curve
};

struct ScrollKeyframe
{
    float time   = 0.0F;
    float speedX = 0.0F;
};

struct ScrollSettings
{
    ScrollMode mode = ScrollMode::Constant;
    float speedX    = 0.0F;
    std::vector<ScrollKeyframe> curve;
};

struct CameraBounds
{
    float minX = 0.0F;
    float maxX = 0.0F;
    float minY = 0.0F;
    float maxY = 0.0F;
};

enum class TriggerType : std::uint8_t
{
    Time,
    Distance,
    SpawnDead,
    BossDead,
    EnemyCountAtMost,
    CheckpointReached,
    HpBelow,
    AllOf,
    AnyOf,
    PlayerInZone,
    PlayersReady
};

struct Trigger
{
    TriggerType type = TriggerType::Time;
    float time       = 0.0F;
    float distance   = 0.0F;
    std::string spawnId;
    std::string bossId;
    std::string checkpointId;
    std::int32_t count = 0;
    std::int32_t value = 0;
    std::vector<Trigger> triggers;
    std::optional<CameraBounds> zone;
    bool requireAllPlayers = false;
};

struct RepeatSpec
{
    float interval = 0.0F;
    std::optional<std::int32_t> count;
    std::optional<Trigger> until;
};

enum class WaveType : std::uint8_t
{
    Line,
    Stagger,
    Triangle,
    Serpent,
    Cross
};

struct WaveDefinition
{
    WaveType type = WaveType::Line;
    std::string enemy;
    std::string patternId;
    float spawnX           = 0.0F;
    float startY           = 0.0F;
    float deltaY           = 0.0F;
    std::int32_t count     = 0;
    float spacing          = 0.0F;
    float apexY            = 0.0F;
    float rowHeight        = 0.0F;
    std::int32_t layers    = 0;
    float horizontalStep   = 0.0F;
    float stepY            = 0.0F;
    float amplitudeX       = 0.0F;
    float stepTime         = 0.0F;
    float centerX          = 0.0F;
    float centerY          = 0.0F;
    float step             = 0.0F;
    std::int32_t armLength = 0;
    std::optional<std::int32_t> health;
    std::optional<Vec2f> scale;
    std::optional<bool> shootingEnabled;
};

struct SpawnObstacleSettings
{
    std::string obstacle;
    std::string spawnId;
    float x = 0.0F;
    std::optional<float> y;
    std::optional<ObstacleAnchor> anchor;
    std::optional<float> margin;
    std::optional<std::int32_t> health;
    std::optional<Vec2f> scale;
    std::optional<float> speedX;
    std::optional<float> speedY;
};

struct SpawnBossSettings
{
    std::string bossId;
    std::string spawnId;
    Vec2f spawn;
};

struct CheckpointDefinition
{
    std::string checkpointId;
    Vec2f respawn;
};

enum class EventType : std::uint8_t
{
    SpawnWave,
    SpawnObstacle,
    SpawnBoss,
    SetScroll,
    SetBackground,
    SetMusic,
    SetCameraBounds,
    SetPlayerBounds,
    ClearPlayerBounds,
    GateOpen,
    GateClose,
    Checkpoint
};

struct LevelEvent
{
    EventType type = EventType::SpawnWave;
    std::string id;
    Trigger trigger;
    std::optional<RepeatSpec> repeat;
    std::optional<WaveDefinition> wave;
    std::optional<SpawnObstacleSettings> obstacle;
    std::optional<SpawnBossSettings> boss;
    std::optional<ScrollSettings> scroll;
    std::optional<std::string> backgroundId;
    std::optional<std::string> musicId;
    std::optional<CameraBounds> cameraBounds;
    std::optional<CameraBounds> playerBounds;
    std::optional<std::string> gateId;
    std::optional<CheckpointDefinition> checkpoint;
};

struct LevelSegment
{
    std::string id;
    ScrollSettings scroll;
    std::vector<LevelEvent> events;
    Trigger exit;
    bool bossRoom = false;
    std::optional<CameraBounds> cameraBounds;
};

struct BossPhase
{
    std::string id;
    Trigger trigger;
    std::vector<LevelEvent> events;
};

struct BossDefinition
{
    std::uint16_t typeId = 0;
    HitboxComponent hitbox;
    ColliderComponent collider;
    std::int32_t health = 1;
    std::int32_t score  = 0;
    Vec2f scale{1.0F, 1.0F};
    std::optional<std::string> patternId;
    std::optional<EnemyShootingComponent> shooting;
    std::vector<BossPhase> phases;
    std::vector<LevelEvent> onDeath;
};

struct LevelData
{
    std::int32_t schemaVersion = 1;
    std::int32_t levelId       = 0;
    LevelMeta meta;
    std::vector<LevelArchetype> archetypes;
    std::vector<PatternDefinition> patterns;
    LevelTemplates templates;
    std::unordered_map<std::string, BossDefinition> bosses;
    std::vector<LevelSegment> segments;
};
