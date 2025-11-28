#include "events/DamageEvent.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/DestructionSystem.hpp"

#include <gtest/gtest.h>

static Collision makeCollision(EntityId a, EntityId b)
{
    return Collision{a, b};
}

TEST(CollisionDestructionIntegration, MissileHitsEnemyAndDestroys)
{
    EventBus bus;
    Registry registry;
    EntityId missile = registry.createEntity();
    EntityId enemy   = registry.createEntity();

    registry.emplace<TransformComponent>(missile, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<HitboxComponent>(missile, HitboxComponent::create(1.0F, 1.0F, 0.0F, 0.0F, true));
    registry.emplace<MissileComponent>(missile, MissileComponent{5, 1.0F, true});
    registry.emplace<OwnershipComponent>(missile, OwnershipComponent::create(1, 0));
    registry.emplace<HealthComponent>(enemy, HealthComponent::create(5));
    registry.emplace<TransformComponent>(enemy, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<HitboxComponent>(enemy, HitboxComponent::create(1.0F, 1.0F, 0.0F, 0.0F, true));

    CollisionSystem collision;
    DamageSystem damage(bus);
    DestructionSystem destroy(bus);

    auto collisions = collision.detect(registry);
    damage.apply(registry, collisions);
    std::vector<EntityId> toDestroy;
    for (auto& c : collisions) {
        if (registry.isAlive(c.b) && registry.has<HealthComponent>(c.b) &&
            registry.get<HealthComponent>(c.b).current <= 0)
            toDestroy.push_back(c.b);
        if (registry.isAlive(c.a) && registry.has<MissileComponent>(c.a))
            registry.destroyEntity(c.a);
    }
    destroy.update(registry, toDestroy);

    EXPECT_FALSE(registry.isAlive(enemy));
}

TEST(CollisionDestructionIntegration, EnemySurvivesIfHealthRemains)
{
    EventBus bus;
    Registry registry;
    EntityId missile = registry.createEntity();
    EntityId enemy   = registry.createEntity();

    registry.emplace<TransformComponent>(missile, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<HitboxComponent>(missile, HitboxComponent::create(1.0F, 1.0F, 0.0F, 0.0F, true));
    registry.emplace<MissileComponent>(missile, MissileComponent{2, 1.0F, true});
    registry.emplace<HealthComponent>(enemy, HealthComponent::create(5));
    registry.emplace<TransformComponent>(enemy, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<HitboxComponent>(enemy, HitboxComponent::create(1.0F, 1.0F, 0.0F, 0.0F, true));

    CollisionSystem collision;
    DamageSystem damage(bus);
    DestructionSystem destroy(bus);

    auto collisions = collision.detect(registry);
    damage.apply(registry, collisions);
    std::vector<EntityId> toDestroy;
    for (auto& c : collisions) {
        if (registry.isAlive(c.a) && registry.has<MissileComponent>(c.a))
            registry.destroyEntity(c.a);
        if (registry.isAlive(c.b) && registry.has<HealthComponent>(c.b) &&
            registry.get<HealthComponent>(c.b).current <= 0)
            toDestroy.push_back(c.b);
    }
    destroy.update(registry, toDestroy);

    EXPECT_TRUE(registry.isAlive(enemy));
    if (registry.isAlive(enemy)) {
        EXPECT_EQ(registry.get<HealthComponent>(enemy).current, 3);
    }
}

TEST(CollisionDestructionIntegration, MultipleCollisionsProcessed)
{
    EventBus bus;
    Registry registry;
    EntityId m1       = registry.createEntity();
    EntityId m2       = registry.createEntity();
    EntityId e1       = registry.createEntity();
    EntityId e2       = registry.createEntity();
    auto setupMissile = [&](EntityId id, float x) {
        registry.emplace<TransformComponent>(id, TransformComponent::create(x, 0.0F));
        registry.emplace<HitboxComponent>(id, HitboxComponent::create(1.0F, 1.0F, 0.0F, 0.0F, true));
        registry.emplace<MissileComponent>(id, MissileComponent{3, 1.0F, true});
        registry.emplace<OwnershipComponent>(id, OwnershipComponent::create(id, 0));
    };
    auto setupEnemy = [&](EntityId id, float x) {
        registry.emplace<TransformComponent>(id, TransformComponent::create(x, 0.0F));
        registry.emplace<HitboxComponent>(id, HitboxComponent::create(1.0F, 1.0F, 0.0F, 0.0F, true));
        registry.emplace<HealthComponent>(id, HealthComponent::create(3));
    };
    setupMissile(m1, 0.0F);
    setupMissile(m2, 2.0F);
    setupEnemy(e1, 0.0F);
    setupEnemy(e2, 2.0F);

    CollisionSystem collision;
    DamageSystem damage(bus);
    DestructionSystem destroy(bus);

    auto collisions = collision.detect(registry);
    damage.apply(registry, collisions);
    std::vector<EntityId> toDestroy;
    for (auto& c : collisions) {
        if (registry.isAlive(c.a) && registry.has<MissileComponent>(c.a))
            registry.destroyEntity(c.a);
        if (registry.isAlive(c.b) && registry.has<HealthComponent>(c.b) &&
            registry.get<HealthComponent>(c.b).current <= 0)
            toDestroy.push_back(c.b);
    }
    destroy.update(registry, toDestroy);

    EXPECT_FALSE(registry.isAlive(e1));
    EXPECT_FALSE(registry.isAlive(e2));
}
