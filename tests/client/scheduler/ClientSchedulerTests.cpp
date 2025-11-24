#include "scheduler/ClientScheduler.hpp"

#include <gtest/gtest.h>

namespace
{
    class DummySystem : public ISystem
    {
      public:
        void update(Registry&, float) override
        {
            ++updateCount;
        }
        ~DummySystem() override
        {
            destroyed = true;
        }

        int updateCount = 0;
        bool destroyed  = false;
    };

    class InitSystem : public ISystem
    {
      public:
        void initialize() override
        {
            initialized = true;
        }
        void update(Registry&, float) override {}
        void cleanup() override
        {
            cleanedUp = true;
        }

        bool initialized = false;
        bool cleanedUp   = false;
    };
} // namespace

TEST(ClientScheduler, CallsUpdateOnSystems)
{
    ClientScheduler scheduler;
    auto system = std::make_shared<DummySystem>();
    scheduler.addSystem(system);

    Registry registry;
    scheduler.update(registry, 0.016F);
    scheduler.update(registry, 0.016F);

    EXPECT_EQ(system->updateCount, 2);
}

TEST(ClientScheduler, CallsInitializeAndCleanup)
{
    ClientScheduler scheduler;
    auto system = std::make_shared<InitSystem>();
    scheduler.addSystem(system);

    Registry registry;
    scheduler.update(registry, 0.016F);
    scheduler.stop();

    EXPECT_TRUE(system->initialized);
    EXPECT_TRUE(system->cleanedUp);
}

TEST(ClientScheduler, AddSystemRejectsNullptr)
{
    ClientScheduler scheduler;
    EXPECT_THROW(scheduler.addSystem(nullptr), std::invalid_argument);
}

TEST(ClientScheduler, ExecutesSystemsInOrder)
{
    struct OrderSystem : public ISystem
    {
        explicit OrderSystem(int id, std::vector<int>& log) : id(id), log(log) {}
        void update(Registry&, float) override
        {
            log.push_back(id);
        }
        int id;
        std::vector<int>& log;
    };

    std::vector<int> log;
    ClientScheduler scheduler;
    scheduler.addSystem(std::make_shared<OrderSystem>(1, log));
    scheduler.addSystem(std::make_shared<OrderSystem>(2, log));

    Registry registry;
    scheduler.update(registry, 0.0F);

    ASSERT_EQ(log.size(), 2u);
    EXPECT_EQ(log[0], 1);
    EXPECT_EQ(log[1], 2);
}

TEST(ClientScheduler, CleanupCalledInReverseOrder)
{
    struct CleanupSystem : public ISystem
    {
        explicit CleanupSystem(int id, std::vector<int>& log) : id(id), log(log) {}
        void update(Registry&, float) override {}
        void cleanup() override
        {
            log.push_back(id);
        }
        int id;
        std::vector<int>& log;
    };

    std::vector<int> log;
    ClientScheduler scheduler;
    scheduler.addSystem(std::make_shared<CleanupSystem>(1, log));
    scheduler.addSystem(std::make_shared<CleanupSystem>(2, log));

    Registry registry;
    scheduler.update(registry, 0.0F);
    scheduler.stop();

    ASSERT_EQ(log.size(), 2u);
    EXPECT_EQ(log[0], 2);
    EXPECT_EQ(log[1], 1);
}

TEST(ClientScheduler, MultipleUpdatesAccumulate)
{
    ClientScheduler scheduler;
    auto system = std::make_shared<DummySystem>();
    scheduler.addSystem(system);

    Registry registry;
    scheduler.update(registry, 0.01F);
    scheduler.update(registry, 0.02F);
    scheduler.update(registry, 0.03F);

    EXPECT_EQ(system->updateCount, 3);
}

TEST(ClientScheduler, StopClearsSystems)
{
    ClientScheduler scheduler;
    auto system = std::make_shared<DummySystem>();
    scheduler.addSystem(system);

    Registry registry;
    scheduler.stop();
    scheduler.update(registry, 0.01F);

    EXPECT_EQ(system->updateCount, 0);
}
