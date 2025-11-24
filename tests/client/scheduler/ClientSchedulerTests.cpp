#include "scheduler/ClientScheduler.hpp"

#include <gtest/gtest.h>

namespace
{
    class DummySystem : public ISystem
    {
      public:
        void update(Registry&, float) override { ++updateCount; }
        ~DummySystem() override { destroyed = true; }

        int updateCount = 0;
        bool destroyed  = false;
    };

    class InitSystem : public ISystem
    {
      public:
        void initialize() override { initialized = true; }
        void update(Registry&, float) override {}
        void cleanup() override { cleanedUp = true; }

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
