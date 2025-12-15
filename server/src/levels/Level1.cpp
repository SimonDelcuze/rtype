#include "levels/Level1.hpp"

#include "server/ObstacleLibrary.hpp"
#include "server/WaveLibrary.hpp"

std::vector<MovementComponent> Level1::makePatterns() const
{
    return {MovementComponent::linear(140.0F),
            MovementComponent::sine(160.0F, 120.0F, 0.5F),
            MovementComponent::zigzag(150.0F, 90.0F, 1.0F),
            MovementComponent::sine(130.0F, 180.0F, 0.6F, 0.5F),
            MovementComponent::linear(200.0F),
            MovementComponent::zigzag(120.0F, 150.0F, 0.85F)};
}

std::vector<SpawnEvent> Level1::buildTimeline(const HitboxComponent& hitbox,
                                              const EnemyShootingComponent& shooting) const
{
    struct WavePlan
    {
        std::vector<SpawnEvent> events;
        float offset               = 0.0F;
        float scale                = 1.0F;
        std::size_t shooterModulo = 1;
    };

    const float spawnX = 1220.0F;
    auto applyShooterRatio = [](std::vector<SpawnEvent>& wave, std::size_t keepShootingEvery) {
        if (wave.empty() || keepShootingEvery == 0)
            return;
        for (std::size_t i = 0; i < wave.size(); ++i) {
            if ((i % keepShootingEvery) != 0) {
                wave[i].shootingEnabled = false;
            }
        }
    };
    auto applyScale = [](std::vector<SpawnEvent>& wave, float scale) {
        for (auto& ev : wave) {
            ev.scaleX = scale;
            ev.scaleY = scale;
        }
    };

    std::vector<WavePlan> waves;
    waves.push_back(
        {Waves::stagger(0.0F, spawnX, 140.0F, 55.0F, 6, 0, 1, hitbox, shooting, 0.55F), 1.0F, 1.6F, 3});
    waves.push_back({Waves::triangle(0.0F, spawnX, 260.0F, 42.0F, 3, 38.0F, 1, 1, hitbox, shooting), 5.5F, 1.9F,
                     4});
    waves.push_back(
        {Waves::serpent(0.0F, spawnX, 110.0F, 30.0F, 8, 110.0F, 0.4F, 2, 1, hitbox, shooting), 9.0F, 1.8F, 3});
    waves.push_back(
        {Waves::cross(0.0F, spawnX - 20.0F, 320.0F, 48.0F, 3, 3, 2, hitbox, shooting), 13.5F, 2.2F, 2});
    waves.push_back({Waves::line(0.0F, spawnX, 80.0F, 90.0F, 4, 4, 1, hitbox, shooting), 18.0F, 1.7F, 3});
    waves.push_back({Waves::line(0.0F, spawnX, 520.0F, -90.0F, 4, 4, 1, hitbox, shooting), 18.8F, 1.7F, 3});
    waves.push_back(
        {Waves::triangle(0.0F, spawnX - 30.0F, 200.0F, 36.0F, 3, 34.0F, 5, 2, hitbox, shooting), 22.5F, 2.2F, 3});
    waves.push_back(
        {Waves::serpent(0.0F, spawnX, 160.0F, 28.0F, 8, 140.0F, 0.35F, 3, 2, hitbox, shooting), 29.0F, 2.0F, 2});
    waves.push_back(
        {Waves::stagger(0.0F, spawnX, 340.0F, -35.0F, 8, 4, 1, hitbox, shooting, 0.32F), 35.5F, 1.8F, 2});
    waves.push_back(
        {Waves::cross(0.0F, spawnX - 40.0F, 300.0F, 44.0F, 3, 5, 3, hitbox, shooting), 42.0F, 2.5F, 2});
    waves.push_back({Waves::line(0.0F, spawnX, 120.0F, 70.0F, 6, 0, 1, hitbox, shooting), 48.0F, 1.6F, 3});

    std::vector<SpawnEvent> out;
    for (auto& wave : waves) {
        applyShooterRatio(wave.events, wave.shooterModulo);
        applyScale(wave.events, wave.scale);
        auto shifted = offsetWave(wave.events, wave.offset);
        out.insert(out.end(), shifted.begin(), shifted.end());
    }
    return out;
}

LevelScript Level1::buildScript() const
{
    LevelScript level{};
    level.patterns = makePatterns();

    const HitboxComponent hitbox       = HitboxComponent::create(50.0F, 50.0F, 0.0F, 0.0F, true);
    const HitboxComponent topHitbox    = HitboxComponent::create(147.0F, 23.0F, 0.0F, 0.0F, true);
    const HitboxComponent midHitbox    = HitboxComponent::create(105.0F, 47.0F, 0.0F, 0.0F, true);
    const HitboxComponent bottomHitbox = HitboxComponent::create(146.0F, 40.0F, 0.0F, 0.0F, true);
    const EnemyShootingComponent shot  = EnemyShootingComponent::create(1.4F, 320.0F, 6, 3.2F);
    const std::vector<std::array<float, 2>> topHull{{{0.0F, 0.0F},
                                                     {146.0F, 0.0F},
                                                     {146.0F, 4.0F},
                                                     {144.0F, 7.0F},
                                                     {139.0F, 14.0F},
                                                     {137.0F, 16.0F},
                                                     {129.0F, 22.0F},
                                                     {24.0F, 22.0F},
                                                     {4.0F, 6.0F},
                                                     {0.0F, 2.0F}}};
    const std::vector<std::array<float, 2>> midHull{{{0.0F, 24.0F},
                                                     {2.0F, 20.0F},
                                                     {8.0F, 10.0F},
                                                     {10.0F, 8.0F},
                                                     {19.0F, 2.0F},
                                                     {21.0F, 1.0F},
                                                     {72.0F, 1.0F},
                                                     {90.0F, 6.0F},
                                                     {93.0F, 7.0F},
                                                     {101.0F, 11.0F},
                                                     {104.0F, 14.0F},
                                                     {104.0F, 46.0F},
                                                     {21.0F, 46.0F},
                                                     {19.0F, 45.0F},
                                                     {11.0F, 39.0F},
                                                     {1.0F, 29.0F},
                                                     {0.0F, 27.0F}}};
    const std::vector<std::array<float, 2>> bottomHull{{{0.0F, 35.0F},
                                                        {1.0F, 33.0F},
                                                        {6.0F, 26.0F},
                                                        {8.0F, 24.0F},
                                                        {16.0F, 18.0F},
                                                        {18.0F, 17.0F},
                                                        {71.0F, 0.0F},
                                                        {80.0F, 0.0F},
                                                        {83.0F, 1.0F},
                                                        {119.0F, 17.0F},
                                                        {125.0F, 21.0F},
                                                        {138.0F, 30.0F},
                                                        {143.0F, 34.0F},
                                                        {145.0F, 39.0F},
                                                        {0.0F, 39.0F}}};

    level.spawns    = buildTimeline(hitbox, shot);
    level.obstacles = buildObstacles(topHitbox, midHitbox, bottomHitbox, topHull, midHull, bottomHull);
    return level;
}

std::vector<ObstacleSpawn> Level1::buildObstacles(const HitboxComponent& topHitbox, const HitboxComponent& midHitbox,
                                                  const HitboxComponent& bottomHitbox,
                                                  const std::vector<std::array<float, 2>>& topHull,
                                                  const std::vector<std::array<float, 2>>& midHull,
                                                  const std::vector<std::array<float, 2>>& bottomHull) const
{
    std::vector<ObstacleSpawn> obstacles;
    auto topCollider = ColliderComponent::polygon(topHull, topHitbox.offsetX, topHitbox.offsetY, topHitbox.isActive);
    auto midCollider = ColliderComponent::polygon(midHull, midHitbox.offsetX, midHitbox.offsetY, midHitbox.isActive);
    auto bottomCollider =
        ColliderComponent::polygon(bottomHull, bottomHitbox.offsetX, bottomHitbox.offsetY, bottomHitbox.isActive);
    constexpr float spawnX = 1280.0F + 200.0F;
    obstacles.push_back(Obstacles::top(3.0F, spawnX, topHitbox, 28, 0.0F, -45.0F, 9, topCollider, 1.6F, 1.6F));
    obstacles.push_back(
        Obstacles::bottom(6.5F, spawnX, bottomHitbox, 32, 0.0F, -55.0F, 11, bottomCollider, 1.9F, 1.9F));
    obstacles.push_back(Obstacles::at(9.5F, spawnX, 260.0F, midHitbox, 30, -60.0F, 10, midCollider, 1.4F, 1.4F));
    obstacles.push_back(Obstacles::top(13.2F, spawnX, topHitbox, 40, 0.0F, -50.0F, 9, topCollider, 2.2F, 2.2F));
    obstacles.push_back(
        Obstacles::bottom(15.0F, spawnX, bottomHitbox, 40, 0.0F, -50.0F, 11, bottomCollider, 2.0F, 2.0F));
    obstacles.push_back(Obstacles::at(21.0F, spawnX, 360.0F, midHitbox, 28, -65.0F, 10, midCollider, 1.6F, 1.6F));
    obstacles.push_back(Obstacles::top(25.0F, spawnX, topHitbox, 35, 0.0F, -55.0F, 9, topCollider, 2.3F, 2.3F));
    obstacles.push_back(
        Obstacles::bottom(28.0F, spawnX, bottomHitbox, 35, 0.0F, -55.0F, 11, bottomCollider, 2.3F, 2.3F));
    obstacles.push_back(Obstacles::at(34.0F, spawnX, 200.0F, midHitbox, 30, -70.0F, 10, midCollider, 1.8F, 1.8F));
    obstacles.push_back(Obstacles::at(40.0F, spawnX, 430.0F, midHitbox, 45, -60.0F, 10, midCollider, 2.0F, 2.0F));
    return obstacles;
}
