#include "components/HealthComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"
#include "ecs/Registry.hpp"
#include "rollback/ClientRollbackHandler.hpp"

#include <gtest/gtest.h>

class ClientRollbackHandlerTest : public ::testing::Test
{
  protected:
    Registry registry;
    ClientRollbackHandler handler;
};

TEST_F(ClientRollbackHandlerTest, CaptureAndChecksum)
{
    EntityId e1 = registry.createEntity();
    registry.emplace<TransformComponent>(e1, TransformComponent::create(10.0f, 20.0f));
    registry.emplace<HealthComponent>(e1, HealthComponent::create(100));

    std::uint32_t checksum = handler.captureState(10, registry);

    EXPECT_TRUE(handler.hasSnapshot(10));
    auto storedChecksum = handler.getChecksum(10);
    ASSERT_TRUE(storedChecksum.has_value());
    EXPECT_EQ(checksum, *storedChecksum);
}

TEST_F(ClientRollbackHandlerTest, RestoreState)
{
    EntityId e1 = registry.createEntity();
    auto& t1    = registry.emplace<TransformComponent>(e1, TransformComponent::create(10.0f, 20.0f));
    auto& h1    = registry.emplace<HealthComponent>(e1, HealthComponent::create(100));

    handler.captureState(10, registry);

    t1.x         = 50.0f;
    t1.y         = 60.0f;
    h1.current   = 50;
    bool success = handler.handleRollbackRequest(10, 15, registry);
    EXPECT_TRUE(success);

    const auto& restoredT1 = registry.get<TransformComponent>(e1);
    const auto& restoredH1 = registry.get<HealthComponent>(e1);
    EXPECT_FLOAT_EQ(restoredT1.x, 10.0f);
    EXPECT_FLOAT_EQ(restoredT1.y, 20.0f);
    EXPECT_EQ(restoredH1.current, 100);
}

TEST_F(ClientRollbackHandlerTest, RollbackCallback)
{
    bool callbackCalled       = false;
    std::uint64_t targetTick  = 0;
    std::uint64_t currentTick = 0;

    handler.setRollbackCallback([&](std::uint64_t t, std::uint64_t c) {
        callbackCalled = true;
        targetTick     = t;
        currentTick    = c;
    });

    handler.captureState(10, registry);
    handler.handleRollbackRequest(10, 20, registry);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(targetTick, 10);
    EXPECT_EQ(currentTick, 20);
}

TEST_F(ClientRollbackHandlerTest, HandleNonExistentTick)
{
    bool success = handler.handleRollbackRequest(999, 1000, registry);
    EXPECT_FALSE(success);
}

TEST_F(ClientRollbackHandlerTest, SkipsDeletedEntities)
{
    EntityId e1 = registry.createEntity();
    registry.emplace<TransformComponent>(e1, TransformComponent::create(10.0f, 20.0f));

    handler.captureState(10, registry);

    registry.destroyEntity(e1);

    bool success = handler.handleRollbackRequest(10, 15, registry);
    EXPECT_TRUE(success);
}

TEST_F(ClientRollbackHandlerTest, HistoryManagement)
{
    handler.captureState(1, registry);
    EXPECT_EQ(handler.getHistorySize(), 1);

    handler.clear();
    EXPECT_EQ(handler.getHistorySize(), 0);
    EXPECT_FALSE(handler.hasSnapshot(1));
}
