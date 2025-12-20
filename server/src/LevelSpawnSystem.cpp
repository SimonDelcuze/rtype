#include "server/LevelSpawnSystem.hpp"

#include "components/RenderTypeComponent.hpp"
#include "components/TagComponent.hpp"
#include "components/VelocityComponent.hpp"

#include <algorithm>
#include <cmath>

LevelSpawnSystem::LevelSpawnSystem(const LevelData& data, LevelDirector* director, float playfieldHeight)
    : data_(&data), director_(director), playfieldHeight_(playfieldHeight)
{
    patternMap_.clear();
    for (const auto& pattern : data.patterns) {
        patternMap_[pattern.id] = pattern.movement;
    }
}

void LevelSpawnSystem::reset()
{
    time_ = 0.0F;
    pendingEnemies_.clear();
    bossSpawns_.clear();
}

void LevelSpawnSystem::update(Registry& registry, float deltaTime, const std::vector<DispatchedEvent>& events)
{
    time_ += deltaTime;
    spawnPending(registry);
    dispatchEvents(registry, events);
    spawnPending(registry);
}

LevelSpawnSystem::CheckpointState LevelSpawnSystem::captureCheckpointState() const
{
    CheckpointState state;
    state.time           = time_;
    state.pendingEnemies = pendingEnemies_;
    state.bossSpawns     = bossSpawns_;
    return state;
}

void LevelSpawnSystem::restoreCheckpointState(const CheckpointState& state)
{
    time_           = state.time;
    pendingEnemies_ = state.pendingEnemies;
    bossSpawns_     = state.bossSpawns;
}

std::optional<SpawnBossSettings> LevelSpawnSystem::getBossSpawnSettings(const std::string& bossId) const
{
    auto it = bossSpawns_.find(bossId);
    if (it == bossSpawns_.end())
        return std::nullopt;
    return it->second;
}

void LevelSpawnSystem::spawnBossImmediate(Registry& registry, const SpawnBossSettings& settings)
{
    spawnBoss(registry, settings);
}

void LevelSpawnSystem::dispatchEvents(Registry& registry, const std::vector<DispatchedEvent>& events)
{
    for (const auto& dispatched : events) {
        const auto& event = dispatched.event;
        if (event.type == EventType::SpawnWave && event.wave) {
            scheduleWave(event, *event.wave);
        } else if (event.type == EventType::SpawnObstacle && event.obstacle) {
            spawnObstacle(registry, *event.obstacle, event);
        } else if (event.type == EventType::SpawnBoss && event.boss) {
            spawnBoss(registry, *event.boss);
        }
    }
}

void LevelSpawnSystem::spawnPending(Registry& registry)
{
    if (pendingEnemies_.empty())
        return;
    std::sort(pendingEnemies_.begin(), pendingEnemies_.end(),
              [](const PendingEnemySpawn& a, const PendingEnemySpawn& b) { return a.time < b.time; });
    std::size_t index = 0;
    while (index < pendingEnemies_.size() && pendingEnemies_[index].time <= time_) {
        spawnEnemy(registry, pendingEnemies_[index]);
        index++;
    }
    if (index > 0) {
        pendingEnemies_.erase(pendingEnemies_.begin(), pendingEnemies_.begin() + static_cast<std::ptrdiff_t>(index));
    }
}

void LevelSpawnSystem::scheduleWave(const LevelEvent& event, const WaveDefinition& wave)
{
    auto patternIt = patternMap_.find(wave.patternId);
    if (patternIt == patternMap_.end())
        return;
    auto enemyIt = data_->templates.enemies.find(wave.enemy);
    if (enemyIt == data_->templates.enemies.end())
        return;

    const EnemyTemplate& enemy        = enemyIt->second;
    const MovementComponent& movement = patternIt->second;
    const std::string spawnGroupId    = event.id;

    if (wave.type == WaveType::Line) {
        for (std::int32_t i = 0; i < wave.count; ++i) {
            float y = wave.startY + wave.deltaY * static_cast<float>(i);
            enqueueEnemySpawn(0.0F, enemy, movement, wave.spawnX, y, wave, spawnGroupId);
        }
        return;
    }
    if (wave.type == WaveType::Stagger) {
        for (std::int32_t i = 0; i < wave.count; ++i) {
            float y = wave.startY + wave.deltaY * static_cast<float>(i);
            float t = wave.spacing * static_cast<float>(i);
            enqueueEnemySpawn(t, enemy, movement, wave.spawnX, y, wave, spawnGroupId);
        }
        return;
    }
    if (wave.type == WaveType::Triangle) {
        for (std::int32_t layer = 0; layer < wave.layers; ++layer) {
            float y            = wave.apexY + wave.rowHeight * static_cast<float>(layer);
            std::int32_t count = 1 + 2 * layer;
            float startLeft    = -wave.horizontalStep * static_cast<float>(layer);
            for (std::int32_t i = 0; i < count; ++i) {
                float x = wave.spawnX + startLeft + wave.horizontalStep * static_cast<float>(i);
                enqueueEnemySpawn(0.0F, enemy, movement, x, y, wave, spawnGroupId);
            }
        }
        return;
    }
    if (wave.type == WaveType::Serpent) {
        for (std::int32_t i = 0; i < wave.count; ++i) {
            float t = wave.stepTime * static_cast<float>(i);
            float y = wave.startY + wave.stepY * static_cast<float>(i);
            float x = wave.spawnX + ((i % 2 == 0) ? wave.amplitudeX : -wave.amplitudeX);
            enqueueEnemySpawn(t, enemy, movement, x, y, wave, spawnGroupId);
        }
        return;
    }
    if (wave.type == WaveType::Cross) {
        enqueueEnemySpawn(0.0F, enemy, movement, wave.centerX, wave.centerY, wave, spawnGroupId);
        for (std::int32_t i = 1; i <= wave.armLength; ++i) {
            float d = wave.step * static_cast<float>(i);
            enqueueEnemySpawn(0.0F, enemy, movement, wave.centerX + d, wave.centerY, wave, spawnGroupId);
            enqueueEnemySpawn(0.0F, enemy, movement, wave.centerX - d, wave.centerY, wave, spawnGroupId);
            enqueueEnemySpawn(0.0F, enemy, movement, wave.centerX, wave.centerY + d, wave, spawnGroupId);
            enqueueEnemySpawn(0.0F, enemy, movement, wave.centerX, wave.centerY - d, wave, spawnGroupId);
        }
        return;
    }
}

void LevelSpawnSystem::enqueueEnemySpawn(float timeOffset, const EnemyTemplate& enemy,
                                         const MovementComponent& movement, float x, float y,
                                         const WaveDefinition& wave, const std::string& spawnGroupId)
{
    PendingEnemySpawn spawn;
    spawn.time     = time_ + std::max(0.0F, timeOffset);
    spawn.movement = movement;
    spawn.hitbox   = enemy.hitbox;
    spawn.collider = enemy.collider;
    spawn.health   = wave.health.has_value() ? *wave.health : enemy.health;
    spawn.scale    = wave.scale.has_value() ? *wave.scale : enemy.scale;
    if (wave.shootingEnabled.has_value()) {
        if (*wave.shootingEnabled && enemy.shooting.has_value())
            spawn.shooting = enemy.shooting;
    } else {
        if (enemy.shooting.has_value())
            spawn.shooting = enemy.shooting;
    }
    spawn.typeId       = enemy.typeId;
    spawn.x            = x;
    spawn.y            = y;
    spawn.spawnGroupId = spawnGroupId;
    pendingEnemies_.push_back(spawn);
}

void LevelSpawnSystem::spawnEnemy(Registry& registry, const PendingEnemySpawn& spawn)
{
    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    t.x        = spawn.x;
    t.y        = spawn.y;
    t.scaleX   = spawn.scale.x;
    t.scaleY   = spawn.scale.y;
    registry.emplace<MovementComponent>(e, spawn.movement);
    registry.emplace<VelocityComponent>(e);
    registry.emplace<TagComponent>(e, TagComponent::create(EntityTag::Enemy));
    registry.emplace<HealthComponent>(e, HealthComponent::create(spawn.health));
    registry.emplace<HitboxComponent>(e, spawn.hitbox);
    registry.emplace<ColliderComponent>(e, spawn.collider);
    registry.emplace<RenderTypeComponent>(e, RenderTypeComponent::create(spawn.typeId));
    if (spawn.shooting.has_value()) {
        registry.emplace<EnemyShootingComponent>(e, *spawn.shooting);
    }
    if (director_ != nullptr && !spawn.spawnGroupId.empty()) {
        director_->registerSpawn(spawn.spawnGroupId, e);
    }
}

void LevelSpawnSystem::spawnObstacle(Registry& registry, const SpawnObstacleSettings& settings, const LevelEvent& event)
{
    auto it = data_->templates.obstacles.find(settings.obstacle);
    if (it == data_->templates.obstacles.end())
        return;
    const ObstacleTemplate& tpl = it->second;

    Vec2f scale = tpl.scale;
    if (settings.scale.has_value())
        scale = *settings.scale;

    ObstacleAnchor anchor = tpl.anchor;
    if (settings.anchor.has_value())
        anchor = *settings.anchor;

    float speedX = tpl.speedX;
    float speedY = tpl.speedY;
    if (settings.speedX.has_value())
        speedX = *settings.speedX;
    if (settings.speedY.has_value())
        speedY = *settings.speedY;

    std::int32_t health = tpl.health;
    if (settings.health.has_value())
        health = *settings.health;

    float y = 0.0F;
    if (anchor == ObstacleAnchor::Absolute) {
        y = settings.y.has_value() ? *settings.y : 0.0F;
    } else {
        y = resolveObstacleY(tpl, settings, scale.y);
    }

    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    t.x        = settings.x;
    t.y        = y;
    t.scaleX   = scale.x;
    t.scaleY   = scale.y;
    registry.emplace<TagComponent>(e, TagComponent::create(EntityTag::Obstacle));
    registry.emplace<HealthComponent>(e, HealthComponent::create(health));
    registry.emplace<VelocityComponent>(e, VelocityComponent::create(speedX, speedY));
    registry.emplace<HitboxComponent>(e, tpl.hitbox);
    registry.emplace<ColliderComponent>(e, tpl.collider);
    registry.emplace<RenderTypeComponent>(e, RenderTypeComponent::create(tpl.typeId));

    std::string spawnId = settings.spawnId.empty() ? event.id : settings.spawnId;
    if (director_ != nullptr && !spawnId.empty()) {
        director_->registerSpawn(spawnId, e);
    }
}

void LevelSpawnSystem::spawnBoss(Registry& registry, const SpawnBossSettings& settings)
{
    auto it = data_->bosses.find(settings.bossId);
    if (it == data_->bosses.end())
        return;
    const BossDefinition& boss = it->second;

    bossSpawns_[settings.bossId] = settings;

    EntityId e = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(e);
    t.x        = settings.spawn.x;
    t.y        = settings.spawn.y;
    t.scaleX   = boss.scale.x;
    t.scaleY   = boss.scale.y;
    registry.emplace<TagComponent>(e, TagComponent::create(EntityTag::Enemy));
    registry.emplace<HealthComponent>(e, HealthComponent::create(boss.health));
    registry.emplace<HitboxComponent>(e, boss.hitbox);
    registry.emplace<ColliderComponent>(e, boss.collider);
    registry.emplace<RenderTypeComponent>(e, RenderTypeComponent::create(boss.typeId));

    if (director_ != nullptr) {
        director_->registerBoss(settings.bossId, e);
        if (!settings.spawnId.empty()) {
            director_->registerSpawn(settings.spawnId, e);
        }
    }
}

float LevelSpawnSystem::resolveObstacleY(const ObstacleTemplate& tpl, const SpawnObstacleSettings& settings,
                                         float scaleY) const
{
    ObstacleAnchor anchor = tpl.anchor;
    float margin          = tpl.margin;
    if (settings.anchor.has_value())
        anchor = *settings.anchor;
    if (settings.margin.has_value())
        margin = *settings.margin;

    float scaledHeight = tpl.hitbox.height * scaleY;
    float scaledOffset = tpl.hitbox.offsetY * scaleY;
    if (anchor == ObstacleAnchor::Top)
        return margin - scaledOffset;
    if (anchor == ObstacleAnchor::Bottom)
        return playfieldHeight_ - scaledHeight - scaledOffset - margin;
    return settings.y.has_value() ? *settings.y : 0.0F;
}
