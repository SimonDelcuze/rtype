#include "scheduler/IScheduler.hpp"

#include <gtest/gtest.h>

class MockSystem : public ISystem
{
  public:
    int updateCallCount    = 0;
    int initCallCount      = 0;
    int shutdownCallCount  = 0;
    float lastDeltaTime    = 0.0f;
    Registry* lastRegistry = nullptr;

    void initialize() override
    {
        initCallCount++;
    }

    void update(Registry& registry, float deltaTime) override
    {
        updateCallCount++;
        lastDeltaTime = deltaTime;
        lastRegistry  = &registry;
    }

    void cleanup() override
    {
        shutdownCallCount++;
    }
};

class TestScheduler : public IScheduler
{
  public:
    void addSystem(std::shared_ptr<ISystem> system) override
    {
        if (!system) {
            throw std::invalid_argument("Cannot add null system");
        }
        systems_.push_back(system);
        system->initialize();
    }

    void update(Registry& registry, float deltaTime) override
    {
        for (auto& system : systems_) {
            system->update(registry, deltaTime);
        }
    }

    void stop() override
    {
        for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
            (*it)->cleanup();
        }
        systems_.clear();
    }

    std::size_t systemCount() const
    {
        return systems_.size();
    }

  private:
    std::vector<std::shared_ptr<ISystem>> systems_;
};

class SchedulerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        scheduler = std::make_unique<TestScheduler>();
        registry  = std::make_unique<Registry>();
    }

    void TearDown() override
    {
        scheduler.reset();
        registry.reset();
    }

    std::unique_ptr<TestScheduler> scheduler;
    std::unique_ptr<Registry> registry;
};

TEST_F(SchedulerTest, AddSystemCallsInitialize)
{
    auto system = std::make_shared<MockSystem>();

    EXPECT_EQ(system->initCallCount, 0);

    scheduler->addSystem(system);

    EXPECT_EQ(system->initCallCount, 1);
}

TEST_F(SchedulerTest, AddNullSystemThrows)
{
    EXPECT_THROW(scheduler->addSystem(nullptr), std::invalid_argument);
}

TEST_F(SchedulerTest, UpdateCallsSystemUpdate)
{
    auto system = std::make_shared<MockSystem>();
    scheduler->addSystem(system);

    EXPECT_EQ(system->updateCallCount, 0);

    scheduler->update(*registry, 0.016f);

    EXPECT_EQ(system->updateCallCount, 1);
    EXPECT_FLOAT_EQ(system->lastDeltaTime, 0.016f);
    EXPECT_EQ(system->lastRegistry, registry.get());
}

TEST_F(SchedulerTest, UpdateCallsMultipleSystemsInOrder)
{
    auto system1 = std::make_shared<MockSystem>();
    auto system2 = std::make_shared<MockSystem>();
    auto system3 = std::make_shared<MockSystem>();

    scheduler->addSystem(system1);
    scheduler->addSystem(system2);
    scheduler->addSystem(system3);

    scheduler->update(*registry, 0.016f);

    EXPECT_EQ(system1->updateCallCount, 1);
    EXPECT_EQ(system2->updateCallCount, 1);
    EXPECT_EQ(system3->updateCallCount, 1);
}

TEST_F(SchedulerTest, MultipleUpdateCalls)
{
    auto system = std::make_shared<MockSystem>();
    scheduler->addSystem(system);

    scheduler->update(*registry, 0.016f);
    scheduler->update(*registry, 0.033f);
    scheduler->update(*registry, 0.008f);

    EXPECT_EQ(system->updateCallCount, 3);
    EXPECT_FLOAT_EQ(system->lastDeltaTime, 0.008f);
}

TEST_F(SchedulerTest, StopCallsCleanup)
{
    auto system = std::make_shared<MockSystem>();
    scheduler->addSystem(system);

    EXPECT_EQ(system->shutdownCallCount, 0);

    scheduler->stop();

    EXPECT_EQ(system->shutdownCallCount, 1);
}

TEST_F(SchedulerTest, StopCallsMultipleSystemsInReverseOrder)
{
    auto system1 = std::make_shared<MockSystem>();
    auto system2 = std::make_shared<MockSystem>();
    auto system3 = std::make_shared<MockSystem>();

    scheduler->addSystem(system1);
    scheduler->addSystem(system2);
    scheduler->addSystem(system3);

    scheduler->stop();

    EXPECT_EQ(system1->shutdownCallCount, 1);
    EXPECT_EQ(system2->shutdownCallCount, 1);
    EXPECT_EQ(system3->shutdownCallCount, 1);
}

TEST_F(SchedulerTest, StopClearsSystems)
{
    auto system = std::make_shared<MockSystem>();
    scheduler->addSystem(system);

    EXPECT_EQ(scheduler->systemCount(), 1);

    scheduler->stop();

    EXPECT_EQ(scheduler->systemCount(), 0);
}

TEST_F(SchedulerTest, SystemCountReflectsAddedSystems)
{
    EXPECT_EQ(scheduler->systemCount(), 0);

    auto system1 = std::make_shared<MockSystem>();
    scheduler->addSystem(system1);
    EXPECT_EQ(scheduler->systemCount(), 1);

    auto system2 = std::make_shared<MockSystem>();
    scheduler->addSystem(system2);
    EXPECT_EQ(scheduler->systemCount(), 2);

    auto system3 = std::make_shared<MockSystem>();
    scheduler->addSystem(system3);
    EXPECT_EQ(scheduler->systemCount(), 3);
}

TEST_F(SchedulerTest, UpdateWithNoSystemsDoesNotCrash)
{
    EXPECT_NO_THROW(scheduler->update(*registry, 0.016f));
}

TEST_F(SchedulerTest, StopWithNoSystemsDoesNotCrash)
{
    EXPECT_NO_THROW(scheduler->stop());
}

TEST_F(SchedulerTest, MultipleStopCallsAreSafe)
{
    auto system = std::make_shared<MockSystem>();
    scheduler->addSystem(system);

    scheduler->stop();
    EXPECT_EQ(system->shutdownCallCount, 1);

    EXPECT_NO_THROW(scheduler->stop());
    EXPECT_EQ(system->shutdownCallCount, 1);
}
