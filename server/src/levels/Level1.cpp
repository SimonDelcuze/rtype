#include "levels/Level1.hpp"

#include "server/ObstacleLibrary.hpp"
#include "server/WaveLibrary.hpp"

std::vector<MovementComponent> Level1::makePatterns() const
{
    return {MovementComponent::linear(150.0F), MovementComponent::sine(150.0F, 100.0F, 0.5F),
            MovementComponent::zigzag(150.0F, 80.0F, 1.0F)};
}

std::vector<SpawnEvent> Level1::buildTimeline(const HitboxComponent& hitbox,
                                              const EnemyShootingComponent& shooting) const
{
    const float spawnX = 1200.0F;

    auto wave1 = Waves::triangle(0.0F, spawnX, 320.0F, 25.0F, 3, 35.0F, 0, 1, hitbox, shooting);
    auto wave2 = Waves::serpent(0.0F, spawnX, 260.0F, 22.0F, 8, 80.0F, 0.35F, 1, 1, hitbox, shooting);
    auto wave3 = Waves::cross(0.0F, spawnX - 40.0F, 360.0F, 30.0F, 2, 2, 1, hitbox, shooting);

    auto applyShooterRatio = [](std::vector<SpawnEvent>& wave, std::size_t keepShootingEvery) {
        if (wave.empty() || keepShootingEvery == 0)
            return;
        for (std::size_t i = 0; i < wave.size(); ++i) {
            if ((i % keepShootingEvery) != 0) {
                wave[i].shootingEnabled = false;
            }
        }
    };

    applyShooterRatio(wave1, wave1ShooterModulo_);
    applyShooterRatio(wave2, wave2ShooterModulo_);
    applyShooterRatio(wave3, wave3ShooterModulo_);

    std::vector<std::pair<std::vector<SpawnEvent>, float>> timeline{
        {wave1, wave1Offset_},
        {wave2, wave2Offset_},
        {wave3, wave3Offset_},
    };

    std::vector<SpawnEvent> out;
    for (const auto& [wave, offset] : timeline) {
        auto shifted = offsetWave(wave, offset);
        out.insert(out.end(), shifted.begin(), shifted.end());
    }
    for (auto& ev : out) {
        ev.scaleX = 2.0F;
        ev.scaleY = 2.0F;
    }
    return out;
}

LevelScript Level1::buildScript() const
{
    LevelScript level{};
    level.patterns = makePatterns();

    const HitboxComponent hitbox         = HitboxComponent::create(50.0F, 50.0F, 0.0F, 0.0F, true);
    const HitboxComponent obstacleHitbox = HitboxComponent::create(332.0F, 94.0F, 0.0F, 0.0F, true);
    const EnemyShootingComponent shot    = EnemyShootingComponent::create(1.5F, 300.0F, 5, 3.0F);
    const std::vector<std::array<float, 2>> obstacleHull{{{0.0F, 20.0F},
                                                          {20.0F, 0.0F},
                                                          {312.0F, 0.0F},
                                                          {332.0F, 20.0F},
                                                          {332.0F, 74.0F},
                                                          {312.0F, 94.0F},
                                                          {20.0F, 94.0F},
                                                          {0.0F, 74.0F}}};

    level.spawns     = buildTimeline(hitbox, shot);
    level.obstacles  = buildObstacles(obstacleHitbox, obstacleHull);
    return level;
}

std::vector<ObstacleSpawn> Level1::buildObstacles(const HitboxComponent& hitbox,
                                                  const std::vector<std::array<float, 2>>& hull) const
{
    std::vector<ObstacleSpawn> obstacles;
    auto collider = ColliderComponent::polygon(hull, hitbox.offsetX, hitbox.offsetY, hitbox.isActive);
    obstacles.push_back(Obstacles::top(2.0F, 900.0F, hitbox, 35, 0.0F, -50.0F, 9, collider));
    obstacles.push_back(Obstacles::bottom(4.5F, 1150.0F, hitbox, 35, 0.0F, -50.0F, 9, collider));
    obstacles.push_back(Obstacles::at(6.0F, 750.0F, 300.0F, hitbox, 35, -50.0F, 9, collider));
    return obstacles;
}
