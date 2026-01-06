#include "game/GameInstanceManager.hpp"

#include <atomic>
#include <gtest/gtest.h>
#include <thread>

class GameInstanceManagerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        runningFlag = true;
        manager     = std::make_unique<GameInstanceManager>(kBasePort, kMaxInstances, runningFlag, false, false);
    }

    void TearDown() override
    {
        runningFlag = false;
        manager.reset();
    }

    static constexpr std::uint16_t kBasePort     = 60000;
    static constexpr std::uint32_t kMaxInstances = 5;
    std::atomic<bool> runningFlag                = true;
    std::unique_ptr<GameInstanceManager> manager;
};

TEST_F(GameInstanceManagerTest, CreateFirstInstance)
{
    auto roomId = manager->createInstance();

    ASSERT_TRUE(roomId.has_value());
    EXPECT_EQ(roomId.value(), 1);

    auto* instance = manager->getInstance(1);
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->getRoomId(), 1);
    EXPECT_EQ(instance->getPort(), kBasePort + 1);
}

TEST_F(GameInstanceManagerTest, CreateMultipleInstances)
{
    auto room1 = manager->createInstance();
    auto room2 = manager->createInstance();
    auto room3 = manager->createInstance();

    ASSERT_TRUE(room1.has_value());
    ASSERT_TRUE(room2.has_value());
    ASSERT_TRUE(room3.has_value());

    EXPECT_EQ(room1.value(), 1);
    EXPECT_EQ(room2.value(), 2);
    EXPECT_EQ(room3.value(), 3);

    EXPECT_NE(manager->getInstance(1), nullptr);
    EXPECT_NE(manager->getInstance(2), nullptr);
    EXPECT_NE(manager->getInstance(3), nullptr);
}

TEST_F(GameInstanceManagerTest, CreateUpToMaxInstances)
{
    std::vector<std::uint32_t> roomIds;

    for (std::uint32_t i = 0; i < kMaxInstances; ++i) {
        auto roomId = manager->createInstance();
        ASSERT_TRUE(roomId.has_value());
        roomIds.push_back(roomId.value());
    }

    EXPECT_EQ(roomIds.size(), kMaxInstances);

    for (std::uint32_t i = 1; i <= kMaxInstances; ++i) {
        EXPECT_NE(manager->getInstance(i), nullptr);
    }
}

TEST_F(GameInstanceManagerTest, CannotExceedMaxInstances)
{
    for (std::uint32_t i = 0; i < kMaxInstances; ++i) {
        auto roomId = manager->createInstance();
        ASSERT_TRUE(roomId.has_value());
    }

    auto extraRoom = manager->createInstance();
    EXPECT_FALSE(extraRoom.has_value());
}

TEST_F(GameInstanceManagerTest, DestroyInstance)
{
    auto roomId = manager->createInstance();
    ASSERT_TRUE(roomId.has_value());

    auto* instance = manager->getInstance(roomId.value());
    ASSERT_NE(instance, nullptr);

    manager->destroyInstance(roomId.value());

    instance = manager->getInstance(roomId.value());
    EXPECT_EQ(instance, nullptr);
}

TEST_F(GameInstanceManagerTest, DestroyNonExistentInstance)
{
    manager->destroyInstance(999);

    EXPECT_EQ(manager->getInstance(999), nullptr);
}

TEST_F(GameInstanceManagerTest, RecreateAfterDestroy)
{
    auto room1 = manager->createInstance();
    ASSERT_TRUE(room1.has_value());

    manager->destroyInstance(room1.value());

    auto room2 = manager->createInstance();
    ASSERT_TRUE(room2.has_value());
    EXPECT_NE(room1.value(), room2.value());
}

TEST_F(GameInstanceManagerTest, PortAllocationIsCorrect)
{
    auto room1 = manager->createInstance();
    auto room2 = manager->createInstance();
    auto room3 = manager->createInstance();

    ASSERT_TRUE(room1.has_value());
    ASSERT_TRUE(room2.has_value());
    ASSERT_TRUE(room3.has_value());

    auto* instance1 = manager->getInstance(room1.value());
    auto* instance2 = manager->getInstance(room2.value());
    auto* instance3 = manager->getInstance(room3.value());

    ASSERT_NE(instance1, nullptr);
    ASSERT_NE(instance2, nullptr);
    ASSERT_NE(instance3, nullptr);

    EXPECT_EQ(instance1->getPort(), kBasePort + room1.value());
    EXPECT_EQ(instance2->getPort(), kBasePort + room2.value());
    EXPECT_EQ(instance3->getPort(), kBasePort + room3.value());
}

TEST_F(GameInstanceManagerTest, GetNonExistentInstance)
{
    auto* instance = manager->getInstance(999);
    EXPECT_EQ(instance, nullptr);
}

TEST_F(GameInstanceManagerTest, CleanupEmptyInstances)
{
    auto room1 = manager->createInstance();
    auto room2 = manager->createInstance();

    ASSERT_TRUE(room1.has_value());
    ASSERT_TRUE(room2.has_value());

    auto* instance1 = manager->getInstance(room1.value());
    auto* instance2 = manager->getInstance(room2.value());

    ASSERT_NE(instance1, nullptr);
    ASSERT_NE(instance2, nullptr);

    EXPECT_TRUE(instance1->isEmpty());
    EXPECT_TRUE(instance2->isEmpty());

    manager->cleanupEmptyInstances();

    EXPECT_EQ(manager->getInstance(room1.value()), nullptr);
    EXPECT_EQ(manager->getInstance(room2.value()), nullptr);
}

TEST_F(GameInstanceManagerTest, ThreadSafeCreation)
{
    constexpr int kNumThreads = 10;
    std::vector<std::thread> threads;
    std::vector<std::optional<std::uint32_t>> results(kNumThreads);

    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([this, i, &results]() { results[i] = manager->createInstance(); });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    int successCount = 0;
    std::set<std::uint32_t> uniqueRoomIds;

    for (const auto& result : results) {
        if (result.has_value()) {
            successCount++;
            uniqueRoomIds.insert(result.value());
        }
    }

    EXPECT_EQ(successCount, kMaxInstances);
    EXPECT_EQ(uniqueRoomIds.size(), kMaxInstances);
}
