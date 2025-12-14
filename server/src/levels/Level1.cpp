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

    const HitboxComponent hitbox       = HitboxComponent::create(50.0F, 50.0F, 0.0F, 0.0F, true);
    const HitboxComponent topHitbox    = HitboxComponent::create(147.0F, 23.0F, 0.0F, 0.0F, true);
    const HitboxComponent midHitbox    = HitboxComponent::create(105.0F, 47.0F, 0.0F, 0.0F, true);
    const HitboxComponent bottomHitbox = HitboxComponent::create(146.0F, 40.0F, 0.0F, 0.0F, true);
    const EnemyShootingComponent shot  = EnemyShootingComponent::create(1.5F, 300.0F, 5, 3.0F);
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
    obstacles.push_back(Obstacles::top(0.1F, spawnX, topHitbox, 35, 0.0F, -50.0F, 9, topCollider, 2.0F, 2.0F));
    obstacles.push_back(
        Obstacles::bottom(5.0F, spawnX, bottomHitbox, 35, 0.0F, -50.0F, 11, bottomCollider, 3.0F, 3.0F));
    obstacles.push_back(Obstacles::at(7.0F, spawnX, 300.0F, midHitbox, 35, -50.0F, 10, midCollider, 1.0F, 1.0F));
    return obstacles;
}
