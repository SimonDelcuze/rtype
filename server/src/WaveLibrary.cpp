#include "server/WaveLibrary.hpp"

#include <algorithm>

namespace
{
    SpawnEvent makeSpawn(float time, float x, float y, std::size_t pattern, std::int32_t hp,
                         const HitboxComponent& hitbox, const EnemyShootingComponent& shooting)
    {
        SpawnEvent ev{};
        ev.time    = time;
        ev.x       = x;
        ev.y       = y;
        ev.pattern = pattern;
        ev.health  = hp;
        ev.hitbox  = hitbox;
        ev.collider =
            ColliderComponent::box(hitbox.width, hitbox.height, hitbox.offsetX, hitbox.offsetY, hitbox.isActive);
        ev.shooting = shooting;
        return ev;
    }
} // namespace

namespace Waves
{
    std::vector<SpawnEvent> line(float startTime, float spawnX, float startY, float deltaY, int count,
                                 std::size_t patternIndex, std::int32_t hp, const HitboxComponent& hitbox,
                                 const EnemyShootingComponent& shooting)
    {
        std::vector<SpawnEvent> out;
        out.reserve(static_cast<std::size_t>(std::max(0, count)));
        for (int i = 0; i < count; ++i) {
            float y = startY + deltaY * static_cast<float>(i);
            out.push_back(makeSpawn(startTime, spawnX, y, patternIndex, hp, hitbox, shooting));
        }
        return out;
    }

    std::vector<SpawnEvent> stagger(float startTime, float spawnX, float startY, float deltaY, int count,
                                    std::size_t patternIndex, std::int32_t hp, const HitboxComponent& hitbox,
                                    const EnemyShootingComponent& shooting, float spacing)
    {
        std::vector<SpawnEvent> out;
        out.reserve(static_cast<std::size_t>(std::max(0, count)));
        for (int i = 0; i < count; ++i) {
            float y    = startY + deltaY * static_cast<float>(i);
            float time = startTime + spacing * static_cast<float>(i);
            out.push_back(makeSpawn(time, spawnX, y, patternIndex, hp, hitbox, shooting));
        }
        return out;
    }

    std::vector<SpawnEvent> triangle(float startTime, float spawnX, float apexY, float rowHeight, int layers,
                                     float horizontalStep, std::size_t patternIndex, std::int32_t hp,
                                     const HitboxComponent& hitbox, const EnemyShootingComponent& shooting)
    {
        std::vector<SpawnEvent> out;
        int safeLayers = std::max(0, layers);
        for (int layer = 0; layer < safeLayers; ++layer) {
            float y         = apexY + rowHeight * static_cast<float>(layer);
            int rowCount    = 1 + 2 * layer;
            float startLeft = -horizontalStep * static_cast<float>(layer);
            for (int i = 0; i < rowCount; ++i) {
                float xOffset = startLeft + horizontalStep * static_cast<float>(i);
                out.push_back(makeSpawn(startTime, spawnX + xOffset, y, patternIndex, hp, hitbox, shooting));
            }
        }
        return out;
    }

    std::vector<SpawnEvent> serpent(float startTime, float spawnX, float startY, float stepY, int count,
                                    float amplitudeX, float stepTime, std::size_t patternIndex, std::int32_t hp,
                                    const HitboxComponent& hitbox, const EnemyShootingComponent& shooting)
    {
        std::vector<SpawnEvent> out;
        out.reserve(static_cast<std::size_t>(std::max(0, count)));
        for (int i = 0; i < count; ++i) {
            float t = startTime + stepTime * static_cast<float>(i);
            float y = startY + stepY * static_cast<float>(i);
            float x = spawnX + ((i % 2 == 0) ? amplitudeX : -amplitudeX);
            out.push_back(makeSpawn(t, x, y, patternIndex, hp, hitbox, shooting));
        }
        return out;
    }

    std::vector<SpawnEvent> cross(float startTime, float centerX, float centerY, float step, int armLength,
                                  std::size_t patternIndex, std::int32_t hp, const HitboxComponent& hitbox,
                                  const EnemyShootingComponent& shooting)
    {
        std::vector<SpawnEvent> out;
        int safeLen = std::max(0, armLength);
        out.push_back(makeSpawn(startTime, centerX, centerY, patternIndex, hp, hitbox, shooting));
        for (int i = 1; i <= safeLen; ++i) {
            float d = step * static_cast<float>(i);
            out.push_back(makeSpawn(startTime, centerX + d, centerY, patternIndex, hp, hitbox, shooting));
            out.push_back(makeSpawn(startTime, centerX - d, centerY, patternIndex, hp, hitbox, shooting));
            out.push_back(makeSpawn(startTime, centerX, centerY + d, patternIndex, hp, hitbox, shooting));
            out.push_back(makeSpawn(startTime, centerX, centerY - d, patternIndex, hp, hitbox, shooting));
        }
        return out;
    }
} // namespace Waves

std::vector<SpawnEvent> offsetWave(const std::vector<SpawnEvent>& wave, float timeOffset)
{
    std::vector<SpawnEvent> shifted;
    shifted.reserve(wave.size());
    for (const auto& ev : wave) {
        SpawnEvent copy = ev;
        copy.time += timeOffset;
        shifted.push_back(copy);
    }
    return shifted;
}
