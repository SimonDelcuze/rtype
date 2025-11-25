#include "events/DamageEvent.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/DamageSystem.hpp"

#include <gtest/gtest.h>

struct DamageCollector
{
    std::vector<DamageEvent> events;
    void operator()(const DamageEvent& e)
    {
        events.push_back(e);
    }
};

static Collision makeCollision(EntityId a, EntityId b)
{
    return Collision{a, b};
}

TEST(DamageSystem, AppliesMissileDamageToHealth)
{
    EventBus bus;
    DamageCollector collector;
    bus.subscribe<DamageEvent>(std::ref(collector));

    Registry registry;
    EntityId missile = registry.createEntity();
    EntityId target  = registry.createEntity();
    registry.emplace<MissileComponent>(missile, MissileComponent{5, 1.0F, true});
    registry.emplace<HealthComponent>(target, HealthComponent::create(10));
    registry.emplace<OwnershipComponent>(missile, OwnershipComponent::create(42, 0));

    DamageSystem sys(bus);
    sys.apply(registry, {makeCollision(missile, target)});

    auto& h = registry.get<HealthComponent>(target);
    EXPECT_EQ(h.current, 5);
    ASSERT_EQ(collector.events.size(), 1u);
    EXPECT_EQ(collector.events[0].attacker, 42u);
    EXPECT_EQ(collector.events[0].target, target);
    EXPECT_EQ(collector.events[0].amount, 5);
    EXPECT_EQ(collector.events[0].remaining, 5);
}

TEST(DamageSystem, IgnoresNonHealthTargets)
{
    EventBus bus;
    DamageCollector collector;
    bus.subscribe<DamageEvent>(std::ref(collector));

    Registry registry;
    EntityId missile  = registry.createEntity();
    EntityId noHealth = registry.createEntity();
    registry.emplace<MissileComponent>(missile, MissileComponent{3, 1.0F, true});

    DamageSystem sys(bus);
    sys.apply(registry, {makeCollision(missile, noHealth)});

    EXPECT_TRUE(collector.events.empty());
}

TEST(DamageSystem, IgnoresNonMissiles)
{
    EventBus bus;
    DamageCollector collector;
    bus.subscribe<DamageEvent>(std::ref(collector));

    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    registry.emplace<HealthComponent>(b, HealthComponent::create(5));

    DamageSystem sys(bus);
    sys.apply(registry, {makeCollision(a, b)});

    EXPECT_EQ(registry.get<HealthComponent>(b).current, 5);
    EXPECT_TRUE(collector.events.empty());
}

TEST(DamageSystem, AppliesBothDirections)
{
    EventBus bus;
    DamageCollector collector;
    bus.subscribe<DamageEvent>(std::ref(collector));

    Registry registry;
    EntityId m1 = registry.createEntity();
    EntityId m2 = registry.createEntity();
    registry.emplace<MissileComponent>(m1, MissileComponent{2, 1.0F, true});
    registry.emplace<MissileComponent>(m2, MissileComponent{4, 1.0F, true});
    registry.emplace<HealthComponent>(m1, HealthComponent::create(5));
    registry.emplace<HealthComponent>(m2, HealthComponent::create(5));

    DamageSystem sys(bus);
    sys.apply(registry, {makeCollision(m1, m2)});

    EXPECT_EQ(registry.get<HealthComponent>(m1).current, 1);
    EXPECT_EQ(registry.get<HealthComponent>(m2).current, 1);
    EXPECT_EQ(collector.events.size(), 2u);
}

TEST(DamageSystem, CapsDamageAtCurrentHealth)
{
    EventBus bus;
    DamageCollector collector;
    bus.subscribe<DamageEvent>(std::ref(collector));

    Registry registry;
    EntityId missile = registry.createEntity();
    EntityId target  = registry.createEntity();
    registry.emplace<MissileComponent>(missile, MissileComponent{10, 1.0F, true});
    registry.emplace<HealthComponent>(target, HealthComponent::create(6));

    DamageSystem sys(bus);
    sys.apply(registry, {makeCollision(missile, target)});

    EXPECT_EQ(registry.get<HealthComponent>(target).current, 0);
    ASSERT_EQ(collector.events.size(), 1u);
    EXPECT_EQ(collector.events[0].amount, 6);
    EXPECT_EQ(collector.events[0].remaining, 0);
}

TEST(DamageSystem, NoCrashOnDeadEntities)
{
    EventBus bus;
    DamageCollector collector;
    bus.subscribe<DamageEvent>(std::ref(collector));

    Registry registry;
    EntityId missile = registry.createEntity();
    EntityId target  = registry.createEntity();
    registry.emplace<MissileComponent>(missile, MissileComponent{3, 1.0F, true});
    registry.emplace<HealthComponent>(target, HealthComponent::create(5));
    registry.destroyEntity(target);

    DamageSystem sys(bus);
    sys.apply(registry, {makeCollision(missile, target)});

    EXPECT_TRUE(collector.events.empty());
}
