#include "components/InputHistoryComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/ReconciliationSystem.hpp"

#include <gtest/gtest.h>

class ReconciliationSystemTests : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        system = std::make_unique<ReconciliationSystem>();
        system->initialize();
    }

    std::unique_ptr<ReconciliationSystem> system;
};

TEST_F(ReconciliationSystemTests, NoReconciliationWithoutComponents)
{
    Registry registry;
    EntityId entity = registry.createEntity();

    system->reconcile(registry, entity, 100.0F, 100.0F, 1);
}

TEST_F(ReconciliationSystemTests, NoReconciliationWithoutInputHistory)
{
    Registry registry;
    EntityId entity = registry.createEntity();
    registry.emplace<TransformComponent>(entity, TransformComponent::create(50.0F, 50.0F));

    system->reconcile(registry, entity, 100.0F, 100.0F, 1);

    const auto& transform = registry.get<TransformComponent>(entity);
    EXPECT_FLOAT_EQ(transform.x, 50.0F);
    EXPECT_FLOAT_EQ(transform.y, 50.0F);
}

TEST_F(ReconciliationSystemTests, SmallErrorDoesNotReconcile)
{
    Registry registry;
    EntityId entity = registry.createEntity();

    auto& transform = registry.emplace<TransformComponent>(entity, TransformComponent::create(100.1F, 100.1F));
    auto& history   = registry.emplace<InputHistoryComponent>(entity);

    history.pushInput(1, 0, 0.0F, 0.0F, 0.0F);

    system->reconcile(registry, entity, 100.0F, 100.0F, 1);

    EXPECT_FLOAT_EQ(transform.x, 100.1F);
    EXPECT_FLOAT_EQ(transform.y, 100.1F);

    EXPECT_EQ(history.size(), 0u);
}

TEST_F(ReconciliationSystemTests, LargeErrorTriggersReconciliation)
{
    Registry registry;
    EntityId entity = registry.createEntity();

    auto& transform = registry.emplace<TransformComponent>(entity, TransformComponent::create(110.0F, 110.0F));
    auto& history   = registry.emplace<InputHistoryComponent>(entity);

    history.pushInput(1, 0, 0.0F, 0.0F, 0.0F);

    system->reconcile(registry, entity, 100.0F, 100.0F, 1);

    EXPECT_FLOAT_EQ(transform.x, 100.0F);
    EXPECT_FLOAT_EQ(transform.y, 100.0F);

    EXPECT_EQ(history.size(), 0u);
}

TEST_F(ReconciliationSystemTests, ReplayUnacknowledgedInputs)
{
    Registry registry;
    EntityId entity = registry.createEntity();

    auto& transform = registry.emplace<TransformComponent>(entity, TransformComponent::create(110.0F, 110.0F));
    auto& history   = registry.emplace<InputHistoryComponent>(entity);

    history.pushInput(1, static_cast<std::uint16_t>(InputFlag::MoveRight), 0.0F, 0.0F, 0.0F, 0.016F);
    history.pushInput(2, static_cast<std::uint16_t>(InputFlag::MoveRight), 0.0F, 0.0F, 0.0F, 0.016F);
    history.pushInput(3, static_cast<std::uint16_t>(InputFlag::MoveRight), 0.0F, 0.0F, 0.0F, 0.016F);

    system->reconcile(registry, entity, 100.0F, 100.0F, 1);

    EXPECT_NEAR(transform.x, 108.0F, 0.01F);
    EXPECT_FLOAT_EQ(transform.y, 100.0F);

    EXPECT_EQ(history.size(), 2u);
}

TEST_F(ReconciliationSystemTests, ReplayMovementUp)
{
    Registry registry;
    EntityId entity = registry.createEntity();

    auto& transform = registry.emplace<TransformComponent>(entity, TransformComponent::create(0.0F, 0.0F));
    auto& history   = registry.emplace<InputHistoryComponent>(entity);

    history.pushInput(1, static_cast<std::uint16_t>(InputFlag::MoveUp), 0.0F, 0.0F, 0.0F, 0.016F);

    system->reconcile(registry, entity, 100.0F, 100.0F, 0);

    EXPECT_FLOAT_EQ(transform.x, 100.0F);
    EXPECT_NEAR(transform.y, 96.0F, 0.01F);
}

TEST_F(ReconciliationSystemTests, ReplayMovementDown)
{
    Registry registry;
    EntityId entity = registry.createEntity();

    auto& transform = registry.emplace<TransformComponent>(entity, TransformComponent::create(0.0F, 0.0F));
    auto& history   = registry.emplace<InputHistoryComponent>(entity);

    history.pushInput(1, static_cast<std::uint16_t>(InputFlag::MoveDown), 0.0F, 0.0F, 0.0F, 0.016F);

    system->reconcile(registry, entity, 100.0F, 100.0F, 0);

    EXPECT_FLOAT_EQ(transform.x, 100.0F);
    EXPECT_NEAR(transform.y, 104.0F, 0.01F);
}

TEST_F(ReconciliationSystemTests, ReplayMovementLeft)
{
    Registry registry;
    EntityId entity = registry.createEntity();

    auto& transform = registry.emplace<TransformComponent>(entity, TransformComponent::create(0.0F, 0.0F));
    auto& history   = registry.emplace<InputHistoryComponent>(entity);

    history.pushInput(1, static_cast<std::uint16_t>(InputFlag::MoveLeft), 0.0F, 0.0F, 0.0F, 0.016F);

    system->reconcile(registry, entity, 100.0F, 100.0F, 0);

    EXPECT_NEAR(transform.x, 96.0F, 0.01F);
    EXPECT_FLOAT_EQ(transform.y, 100.0F);
}

TEST_F(ReconciliationSystemTests, ReplayDiagonalMovement)
{
    Registry registry;
    EntityId entity = registry.createEntity();

    auto& transform = registry.emplace<TransformComponent>(entity, TransformComponent::create(0.0F, 0.0F));
    auto& history   = registry.emplace<InputHistoryComponent>(entity);

    std::uint16_t upRight =
        static_cast<std::uint16_t>(InputFlag::MoveUp) | static_cast<std::uint16_t>(InputFlag::MoveRight);
    history.pushInput(1, upRight, 0.0F, 0.0F, 0.0F, 0.016F);

    system->reconcile(registry, entity, 100.0F, 100.0F, 0);

    EXPECT_NEAR(transform.x, 102.828F, 0.01F);
    EXPECT_NEAR(transform.y, 97.172F, 0.01F);
}

TEST_F(ReconciliationSystemTests, NoReplayWhenAllInputsAcknowledged)
{
    Registry registry;
    EntityId entity = registry.createEntity();

    auto& transform = registry.emplace<TransformComponent>(entity, TransformComponent::create(110.0F, 110.0F));
    auto& history   = registry.emplace<InputHistoryComponent>(entity);

    history.pushInput(1, static_cast<std::uint16_t>(InputFlag::MoveRight), 0.0F, 0.0F, 0.0F);
    history.pushInput(2, static_cast<std::uint16_t>(InputFlag::MoveRight), 0.0F, 0.0F, 0.0F);

    system->reconcile(registry, entity, 100.0F, 100.0F, 2);

    EXPECT_FLOAT_EQ(transform.x, 100.0F);
    EXPECT_FLOAT_EQ(transform.y, 100.0F);

    EXPECT_EQ(history.size(), 0u);
}

TEST_F(ReconciliationSystemTests, MultipleInputReplay)
{
    Registry registry;
    EntityId entity = registry.createEntity();

    auto& transform = registry.emplace<TransformComponent>(entity, TransformComponent::create(0.0F, 0.0F));
    auto& history   = registry.emplace<InputHistoryComponent>(entity);

    for (std::uint32_t i = 1; i <= 5; ++i) {
        history.pushInput(i, static_cast<std::uint16_t>(InputFlag::MoveRight), 0.0F, 0.0F, 0.0F, 0.016F);
    }

    system->reconcile(registry, entity, 100.0F, 100.0F, 2);

    EXPECT_NEAR(transform.x, 112.0F, 0.01F);
    EXPECT_FLOAT_EQ(transform.y, 100.0F);

    EXPECT_EQ(history.size(), 3u);
}

TEST_F(ReconciliationSystemTests, SkipsDeadEntity)
{
    Registry registry;
    EntityId entity = registry.createEntity();

    auto& transform = registry.emplace<TransformComponent>(entity, TransformComponent::create(50.0F, 50.0F));
    registry.emplace<InputHistoryComponent>(entity);

    registry.destroyEntity(entity);

    system->reconcile(registry, entity, 100.0F, 100.0F, 1);
}
