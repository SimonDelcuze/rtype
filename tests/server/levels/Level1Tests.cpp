#include "levels/Level1.hpp"

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

    float maxTime(const std::vector<SpawnEvent>& spawns)
    {
        float m = 0.0F;
        for (const auto& ev : spawns) {
            m = std::max(m, ev.time);
        }
        return m;
    }
} // namespace

TEST(Level1, WavesFollowTheScript)
{
    Level1 level;
    auto script = level.buildScript();

    const std::vector<WaveExpectation> expected{
        {6, 3, 1, 1.6F, 1.0F},  {9, 4, 1, 1.9F, 5.5F},  {8, 3, 1, 1.8F, 9.0F},
        {13, 2, 2, 2.2F, 13.5F}, {4, 3, 1, 1.7F, 18.0F}, {4, 3, 1, 1.7F, 18.8F},
        {9, 3, 2, 2.2F, 22.5F},  {8, 2, 2, 2.0F, 29.0F}, {8, 2, 1, 1.8F, 35.5F},
        {13, 2, 3, 2.5F, 42.0F}, {6, 3, 1, 1.6F, 48.0F}};

    std::vector<std::size_t> counts;
    counts.reserve(expected.size());
    for (const auto& e : expected) {
        counts.push_back(e.count);
    }

    auto waves = splitByCounts(script.spawns, counts);
    ASSERT_EQ(waves.size(), expected.size());

    auto shooterCount = [](const std::vector<SpawnEvent>& wave) {
        int shooters = 0;
        for (const auto& ev : wave) {
            if (ev.shootingEnabled) {
                shooters++;
            }
        }
        return shooters;
    };

    std::size_t totalSpawns = 0;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto& exp  = expected[i];
        const auto& wave = waves[i];
        totalSpawns += wave.size();

        ASSERT_EQ(wave.size(), exp.count);
        EXPECT_EQ(shooterCount(wave), expectedShooters(wave.size(), exp.shooterModulo));
        for (const auto& ev : wave) {
            EXPECT_EQ(ev.health, exp.health);
            EXPECT_FLOAT_EQ(ev.scaleX, exp.scale);
            EXPECT_FLOAT_EQ(ev.scaleY, exp.scale);
        }
        EXPECT_NEAR(minTime(wave), exp.minTime, 1e-3F);
    }

    EXPECT_EQ(script.spawns.size(), totalSpawns);
    EXPECT_NEAR(maxTime(script.spawns), 48.0F, 1e-3F);
}

TEST(Level1, ObstaclesCoverAnchorsAndTimeline)
{
    Level1 level;
    auto script = level.buildScript();

    EXPECT_EQ(script.obstacles.size(), 10u);

    int top = 0;
    int bottom = 0;
    int absolute = 0;
    float earliest = std::numeric_limits<float>::max();
    float latest   = 0.0F;
    for (const auto& obs : script.obstacles) {
        earliest = std::min(earliest, obs.time);
        latest   = std::max(latest, obs.time);
        switch (obs.anchor) {
            case ObstacleAnchor::Top:
                top++;
                break;
            case ObstacleAnchor::Bottom:
                bottom++;
                break;
            case ObstacleAnchor::Absolute:
                absolute++;
                break;
        }
    }

    EXPECT_EQ(top, 3);
    EXPECT_EQ(bottom, 3);
    EXPECT_EQ(absolute, 4);
    EXPECT_NEAR(earliest, 3.0F, 1e-3F);
    EXPECT_NEAR(latest, 40.0F, 1e-3F);
}
