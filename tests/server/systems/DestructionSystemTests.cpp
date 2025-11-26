#include "events/DestroyEvent.hpp"
#include "systems/DestructionSystem.hpp"

#include <gtest/gtest.h>

struct DestroyCollector
{
    std::vector<EntityId> ids;
    void operator()(const DestroyEvent& e)
    {
        ids.push_back(e.id);
    }
};

TEST(DestructionSystem, DestroysEntitiesAndBroadcasts)
{
    EventBus bus;
    DestroyCollector collector;
    bus.subscribe<DestroyEvent>(std::ref(collector));

    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    registry.emplace<TransformComponent>(a, TransformComponent::create(1.0F, 2.0F));
    registry.emplace<VelocityComponent>(b, VelocityComponent::create(1.0F, 1.0F));

    DestructionSystem sys(bus);
    std::vector<EntityId> list{a, b};
    sys.update(registry, list);

    EXPECT_FALSE(registry.isAlive(a));
    EXPECT_FALSE(registry.isAlive(b));
    ASSERT_EQ(collector.ids.size(), 2u);
    EXPECT_NE(std::find(collector.ids.begin(), collector.ids.end(), a), collector.ids.end());
    EXPECT_NE(std::find(collector.ids.begin(), collector.ids.end(), b), collector.ids.end());
}

TEST(DestructionSystem, IgnoresAlreadyDead)
{
    EventBus bus;
    DestroyCollector collector;
    bus.subscribe<DestroyEvent>(std::ref(collector));

    Registry registry;
    EntityId a = registry.createEntity();
    registry.destroyEntity(a);

    DestructionSystem sys(bus);
    sys.update(registry, {a});

    EXPECT_TRUE(collector.ids.empty());
}

TEST(DestructionSystem, LeavesOtherEntitiesUntouched)
{
    EventBus bus;
    DestroyCollector collector;
    bus.subscribe<DestroyEvent>(std::ref(collector));

    Registry registry;
    EntityId alive = registry.createEntity();
    registry.emplace<HealthComponent>(alive, HealthComponent::create(10));
    EntityId dead = registry.createEntity();
    registry.destroyEntity(dead);

    DestructionSystem sys(bus);
    sys.update(registry, {dead});

    EXPECT_TRUE(registry.isAlive(alive));
    EXPECT_TRUE(collector.ids.empty());
}

TEST(DestructionSystem, MultipleCallsAccumulateEvents)
{
    EventBus bus;
    DestroyCollector collector;
    bus.subscribe<DestroyEvent>(std::ref(collector));

    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    EntityId c = registry.createEntity();

    DestructionSystem sys(bus);
    sys.update(registry, {a});
    sys.update(registry, {b, c});

    EXPECT_FALSE(registry.isAlive(a));
    EXPECT_FALSE(registry.isAlive(b));
    EXPECT_FALSE(registry.isAlive(c));
    EXPECT_EQ(collector.ids.size(), 3u);
    EXPECT_NE(std::find(collector.ids.begin(), collector.ids.end(), a), collector.ids.end());
    EXPECT_NE(std::find(collector.ids.begin(), collector.ids.end(), b), collector.ids.end());
    EXPECT_NE(std::find(collector.ids.begin(), collector.ids.end(), c), collector.ids.end());
}

TEST(DestructionSystem, NoDuplicateEventsForDeadInput)
{
    EventBus bus;
    DestroyCollector collector;
    bus.subscribe<DestroyEvent>(std::ref(collector));

    Registry registry;
    EntityId a = registry.createEntity();
    registry.destroyEntity(a);

    DestructionSystem sys(bus);
    sys.update(registry, {a, a});

    EXPECT_TRUE(collector.ids.empty());
}
