#pragma once

#include "components/Components.hpp"
#include "systems/MonsterSpawnSystem.hpp"

#include <vector>

namespace Waves
{
    std::vector<SpawnEvent> line(float startTime, float spawnX, float startY, float deltaY, int count,
                                 std::size_t patternIndex, std::int32_t hp, const HitboxComponent& hitbox,
                                 const EnemyShootingComponent& shooting);

    std::vector<SpawnEvent> stagger(float startTime, float spawnX, float startY, float deltaY, int count,
                                    std::size_t patternIndex, std::int32_t hp, const HitboxComponent& hitbox,
                                    const EnemyShootingComponent& shooting, float spacing = 0.6F);

    std::vector<SpawnEvent> triangle(float startTime, float spawnX, float apexY, float rowHeight, int layers,
                                     float horizontalStep, std::size_t patternIndex, std::int32_t hp,
                                     const HitboxComponent& hitbox, const EnemyShootingComponent& shooting);

    std::vector<SpawnEvent> serpent(float startTime, float spawnX, float startY, float stepY, int count,
                                    float amplitudeX, float stepTime, std::size_t patternIndex, std::int32_t hp,
                                    const HitboxComponent& hitbox, const EnemyShootingComponent& shooting);

    std::vector<SpawnEvent> cross(float startTime, float centerX, float centerY, float step, int armLength,
                                  std::size_t patternIndex, std::int32_t hp, const HitboxComponent& hitbox,
                                  const EnemyShootingComponent& shooting);
} // namespace

std::vector<SpawnEvent> offsetWave(const std::vector<SpawnEvent>& wave, float timeOffset);
