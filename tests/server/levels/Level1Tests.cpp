#include "levels/Level1.hpp"

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

TEST(Level1, ShooterRatioAndHealth)
{
    Level1 level;
    auto script = level.buildScript();

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
    }
}

TEST(Level1, ScaleAndOffsets)
{
    Level1 level;
    auto script = level.buildScript();

    auto waves = splitByCounts(script.spawns, {9, 8, 9});
    ASSERT_EQ(waves.size(), 3u);

    for (const auto& ev : script.spawns) {
        EXPECT_FLOAT_EQ(ev.scaleX, 2.0F);
        EXPECT_FLOAT_EQ(ev.scaleY, 2.0F);
    }

    auto minTime = [](const std::vector<SpawnEvent>& w) {
        float m = std::numeric_limits<float>::max();
        for (const auto& ev : w) {
            m = std::min(m, ev.time);
        }
        return m;
    };

    EXPECT_NEAR(minTime(waves[0]), 1.0F, 1e-3F);
    EXPECT_NEAR(minTime(waves[1]), 3.0F, 1e-3F);
    EXPECT_NEAR(minTime(waves[2]), 6.0F, 1e-3F);
}
