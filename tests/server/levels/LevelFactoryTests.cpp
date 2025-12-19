#include "levels/LevelFactory.hpp"
#include "server/SpawnConfig.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <limits>

namespace
{
    struct WaveExpectation
    {
        std::size_t count;
        std::size_t shooterModulo;
        std::int32_t health;
        float scale;
        float minTime;
    };

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

    float minTime(const std::vector<SpawnEvent>& w)
    {
        float m = std::numeric_limits<float>::max();
        for (const auto& ev : w) {
            m = std::min(m, ev.time);
        }
        return m;
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

    EXPECT_EQ(setup.patterns.size(), script.patterns.size());
    EXPECT_EQ(setup.spawns.size(), script.spawns.size());
    EXPECT_EQ(setup.obstacles.size(), script.obstacles.size());
}

TEST(LevelFactory, ShooterRatioOffsetsAndHealthFromLevel1)
{
    auto level  = makeLevel(1);
    auto script = level->buildScript();

    const std::vector<WaveExpectation> expected{{6, 1, 1, 1.6F, 1.0F},   {9, 1, 1, 1.9F, 5.5F},  {8, 1, 1, 1.8F, 9.0F},
                                                {13, 1, 2, 2.2F, 13.5F}, {4, 1, 1, 1.7F, 18.0F}, {4, 1, 1, 1.7F, 18.8F},
                                                {9, 1, 2, 2.2F, 22.5F},  {8, 1, 2, 2.0F, 29.0F}, {8, 1, 1, 1.8F, 35.5F},
                                                {13, 1, 3, 2.5F, 42.0F}, {6, 1, 1, 1.6F, 48.0F}};

    std::vector<std::size_t> counts;
    counts.reserve(expected.size());
    for (const auto& e : expected) {
        counts.push_back(e.count);
    }
    std::size_t expectedTotal = 0;
    for (auto c : counts) {
        expectedTotal += c;
    }

    auto waves = splitByCounts(script.spawns, counts);
    ASSERT_EQ(waves.size(), expected.size());
    EXPECT_EQ(script.spawns.size(), expectedTotal);

    auto shooterCount = [](const std::vector<SpawnEvent>& wave) {
        int shooters = 0;
        for (const auto& ev : wave) {
            if (ev.shootingEnabled) {
                shooters++;
            }
        }
        return shooters;
    };

    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto& exp  = expected[i];
        const auto& wave = waves[i];

        EXPECT_EQ(shooterCount(wave), expectedShooters(wave.size(), exp.shooterModulo));
        for (const auto& ev : wave) {
            EXPECT_EQ(ev.health, exp.health);
            EXPECT_FLOAT_EQ(ev.scaleX, exp.scale);
            EXPECT_FLOAT_EQ(ev.scaleY, exp.scale);
        }
        EXPECT_NEAR(minTime(wave), exp.minTime, 1e-3F);
    }
}
