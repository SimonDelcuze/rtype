#include "levels/LevelFactory.hpp"
#include "server/SpawnConfig.hpp"

#include <gtest/gtest.h>

namespace
{
    int expectedShooters(std::size_t count, std::size_t modulo)
    {
        if (modulo == 0)
            return 0;
        return static_cast<int>((count + modulo - 1) / modulo);
    }

    std::vector<std::vector<SpawnEvent>> splitByCounts(std::vector<SpawnEvent> spawns,
                                                       const std::vector<std::size_t>& counts)
    {
        std::vector<std::vector<SpawnEvent>> waves;
        std::sort(spawns.begin(), spawns.end(), [](const auto& a, const auto& b) { return a.time < b.time; });
        std::size_t idx = 0;
        for (std::size_t c : counts) {
            std::vector<SpawnEvent> wave;
            for (std::size_t i = 0; i < c && idx < spawns.size(); ++i, ++idx) {
                wave.push_back(spawns[idx]);
            }
            waves.push_back(std::move(wave));
        }
        return waves;
    }
} // namespace

TEST(LevelFactory, ReturnsLevel1)
{
    auto lvl = makeLevel(1);
    ASSERT_NE(lvl, nullptr);
    EXPECT_EQ(lvl->id(), 1);
}

TEST(SpawnConfig, BuildSpawnSetupForLevel1MatchesLevelScript)
{
    auto setup  = buildSpawnSetupForLevel(1);
    auto level  = makeLevel(1);
    auto script = level->buildScript();

    EXPECT_EQ(setup.first.size(), script.patterns.size());
    EXPECT_EQ(setup.second.size(), script.spawns.size());
}

TEST(LevelFactory, ShooterRatioOffsetsAndHealthFromLevel1)
{
    auto level  = makeLevel(1);
    auto script = level->buildScript();

    auto waves = splitByCounts(script.spawns, {9, 8, 9});
    ASSERT_EQ(waves.size(), 3u);
    auto& wave1 = waves[0];
    auto& wave2 = waves[1];
    auto& wave3 = waves[2];

    auto shooterCount = [](const std::vector<SpawnEvent>& wave) {
        int shooters = 0;
        for (const auto& ev : wave) {
            if (ev.shootingEnabled) {
                shooters++;
            }
        }
        return shooters;
    };

    EXPECT_EQ(shooterCount(wave1), expectedShooters(wave1.size(), 4));
    EXPECT_EQ(shooterCount(wave2), expectedShooters(wave2.size(), 4));
    EXPECT_EQ(shooterCount(wave3), expectedShooters(wave3.size(), 4));

    for (const auto& ev : script.spawns) {
        EXPECT_EQ(ev.health, 1);
        EXPECT_FLOAT_EQ(ev.scaleX, 2.0F);
        EXPECT_FLOAT_EQ(ev.scaleY, 2.0F);
    }
}
