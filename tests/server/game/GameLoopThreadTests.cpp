#include "game/GameLoopThread.hpp"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static bool waitForTicks(std::atomic<int>& counter, int target, int attempts = 200, int stepMs = 5)
{
    for (int i = 0; i < attempts; ++i) {
        if (counter.load() >= target)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
    }
    return false;
}

TEST(GameLoopThread, RunsAtApprox60Hz)
{
    ThreadSafeQueue<ReceivedInput> inputs;
    std::vector<std::chrono::steady_clock::time_point> times;
    std::atomic<int> ticks{0};
    GameLoopThread loop(inputs, [&](const GameLoopThread::TickInputs&) {
        times.push_back(std::chrono::steady_clock::now());
        ticks.fetch_add(1);
    });
    ASSERT_TRUE(loop.start());
    ASSERT_TRUE(waitForTicks(ticks, 20, 500, 2));
    loop.stop();
    ASSERT_GE(times.size(), 2u);
    std::vector<double> intervals;
    for (std::size_t i = 1; i < times.size(); ++i) {
        auto dt = std::chrono::duration<double>(times[i] - times[i - 1]).count();
        intervals.push_back(dt);
    }
    double avg = 0.0;
    for (double v : intervals)
        avg += v;
    avg /= static_cast<double>(intervals.size());
    EXPECT_NEAR(avg, 1.0 / 60.0, 0.01);
}

TEST(GameLoopThread, RunsAtCustomRate)
{
    ThreadSafeQueue<ReceivedInput> inputs;
    std::vector<std::chrono::steady_clock::time_point> times;
    std::atomic<int> ticks{0};
    GameLoopThread loop(
        inputs,
        [&](const GameLoopThread::TickInputs&) {
            times.push_back(std::chrono::steady_clock::now());
            ticks.fetch_add(1);
        },
        30.0);
    ASSERT_TRUE(loop.start());
    ASSERT_TRUE(waitForTicks(ticks, 15, 500, 2));
    loop.stop();
    ASSERT_GE(times.size(), 2u);
    double avg = 0.0;
    int count  = 0;
    for (std::size_t i = 1; i < times.size(); ++i) {
        auto dt = std::chrono::duration<double>(times[i] - times[i - 1]).count();
        avg += dt;
        ++count;
    }
    avg /= static_cast<double>(count);
    EXPECT_NEAR(avg, 1.0 / 30.0, 0.02);
}

TEST(GameLoopThread, DrainsInputQueueEachTick)
{
    ThreadSafeQueue<ReceivedInput> inputs;
    std::atomic<int> processed{0};
    GameLoopThread loop(inputs,
                        [&](const GameLoopThread::TickInputs& batch) { processed += static_cast<int>(batch.size()); });
    ASSERT_TRUE(loop.start());
    for (int i = 0; i < 5; ++i) {
        ReceivedInput ev{};
        ev.input.playerId = static_cast<std::uint32_t>(i + 1);
        inputs.push(ev);
    }
    ASSERT_TRUE(waitForTicks(processed, 5, 200, 2));
    loop.stop();
    EXPECT_EQ(processed.load(), 5);
}

TEST(GameLoopThread, ProcessesNewInputsAcrossTicks)
{
    ThreadSafeQueue<ReceivedInput> inputs;
    std::atomic<int> processed{0};
    GameLoopThread loop(inputs,
                        [&](const GameLoopThread::TickInputs& batch) { processed += static_cast<int>(batch.size()); });
    ASSERT_TRUE(loop.start());
    for (int i = 0; i < 3; ++i) {
        ReceivedInput ev{};
        ev.input.playerId = static_cast<std::uint32_t>(i + 1);
        inputs.push(ev);
    }
    ASSERT_TRUE(waitForTicks(processed, 3, 200, 2));
    for (int i = 0; i < 2; ++i) {
        ReceivedInput ev{};
        ev.input.playerId = static_cast<std::uint32_t>(i + 4);
        inputs.push(ev);
    }
    ASSERT_TRUE(waitForTicks(processed, 5, 200, 2));
    loop.stop();
    EXPECT_EQ(processed.load(), 5);
}

TEST(GameLoopThread, ContinuesWhenWorkSlowerThanTick)
{
    ThreadSafeQueue<ReceivedInput> inputs;
    std::atomic<int> ticks{0};
    GameLoopThread loop(inputs, [&](const GameLoopThread::TickInputs&) {
        std::this_thread::sleep_for(5ms);
        ticks.fetch_add(1);
    });
    ASSERT_TRUE(loop.start());
    std::this_thread::sleep_for(200ms);
    loop.stop();
    EXPECT_GE(ticks.load(), 8);
}

TEST(GameLoopThread, EmptyBatchStillTicks)
{
    ThreadSafeQueue<ReceivedInput> inputs;
    std::atomic<int> ticks{0};
    std::atomic<int> lastSize{0};
    GameLoopThread loop(inputs, [&](const GameLoopThread::TickInputs& batch) {
        lastSize.store(static_cast<int>(batch.size()));
        ticks.fetch_add(1);
    });
    ASSERT_TRUE(loop.start());
    ASSERT_TRUE(waitForTicks(ticks, 5, 300, 2));
    loop.stop();
    EXPECT_EQ(lastSize.load(), 0);
}

TEST(GameLoopThread, StopsHaltsTicks)
{
    ThreadSafeQueue<ReceivedInput> inputs;
    std::atomic<int> ticks{0};
    GameLoopThread loop(inputs, [&](const GameLoopThread::TickInputs&) { ticks.fetch_add(1); });
    ASSERT_TRUE(loop.start());
    ASSERT_TRUE(waitForTicks(ticks, 5, 400, 2));
    loop.stop();
    int before = ticks.load();
    std::this_thread::sleep_for(50ms);
    EXPECT_EQ(ticks.load(), before);
}
