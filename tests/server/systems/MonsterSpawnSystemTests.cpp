#include "systems/MonsterSpawnSystem.hpp"

#include <gtest/gtest.h>

static std::size_t countEnemies(Registry& registry)
{
    std::size_t count = 0;
    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (!registry.isAlive(id))
            continue;
        if (registry.has<TagComponent>(id) && registry.get<TagComponent>(id).hasTag(EntityTag::Enemy))
            ++count;
    }
    return count;
}

static SpawnEvent makeSpawn(float time, float x, float y, std::size_t patternIdx, std::int32_t hp = 50)
{
    SpawnEvent ev{};
    ev.time            = time;
    ev.x               = x;
    ev.y               = y;
    ev.pattern         = patternIdx;
    ev.health          = hp;
    ev.scaleX          = 1.0F;
    ev.scaleY          = 1.0F;
    ev.shootingEnabled = true;
    ev.hitbox          = HitboxComponent::create(10.0F, 10.0F);
    ev.shooting        = EnemyShootingComponent::create(1.0F, 100.0F, 1, 1.0F);
    return ev;
}

TEST(MonsterSpawnSystem, SpawnsAtConfiguredPositions)
{
    Registry registry;
    std::vector<MovementComponent> patterns{MovementComponent::linear(2.0F)};
    std::vector<SpawnEvent> script{makeSpawn(0.1F, 500.0F, -5.0F, 0), makeSpawn(0.1F, 500.0F, 5.0F, 0)};
    MonsterSpawnSystem sys(patterns, script);

    sys.update(registry, 0.2F);

    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (!registry.isAlive(id))
            continue;
        auto& t = registry.get<TransformComponent>(id);
        EXPECT_FLOAT_EQ(t.x, 500.0F);
        EXPECT_GE(t.y, -5.0F);
        EXPECT_LE(t.y, 5.0F);
    }
}

TEST(MonsterSpawnSystem, AssignsMovementPattern)
{
    Registry registry;
    std::vector<MovementComponent> patterns{MovementComponent::linear(3.0F),
                                            MovementComponent::zigzag(4.0F, 1.0F, 2.0F)};
    std::vector<SpawnEvent> script{makeSpawn(0.05F, 0.0F, 0.0F, 0), makeSpawn(0.05F, 0.0F, 0.5F, 1)};
    MonsterSpawnSystem sys(patterns, script);

    sys.update(registry, 0.5F);

    bool hasLinear = false;
    bool hasZigzag = false;
    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (!registry.isAlive(id))
            continue;
        auto& m = registry.get<MovementComponent>(id);
        if (m.pattern == MovementPattern::Linear)
            hasLinear = true;
        if (m.pattern == MovementPattern::Zigzag)
            hasZigzag = true;
    }
    EXPECT_TRUE(hasLinear);
    EXPECT_TRUE(hasZigzag);
}

TEST(MonsterSpawnSystem, NoPatternsNoSpawn)
{
    Registry registry;
    std::vector<MovementComponent> patterns;
    std::vector<SpawnEvent> script{makeSpawn(0.1F, 0.0F, 0.0F, 0)};
    MonsterSpawnSystem sys(patterns, script);

    sys.update(registry, 1.0F);

    EXPECT_EQ(countEnemies(registry), 0u);
}

TEST(MonsterSpawnSystem, MultipleSpawnsInSingleTick)
{
    Registry registry;
    std::vector<MovementComponent> patterns{MovementComponent::sine(1.0F, 1.0F, 1.0F, 0.0F)};
    std::vector<SpawnEvent> script{makeSpawn(0.1F, 50.0F, 0.0F, 0), makeSpawn(0.15F, 50.0F, 0.5F, 0),
                                   makeSpawn(0.2F, 50.0F, 1.0F, 0), makeSpawn(0.25F, 50.0F, 1.5F, 0),
                                   makeSpawn(0.3F, 50.0F, 2.0F, 0)};
    MonsterSpawnSystem sys(patterns, script);

    sys.update(registry, 1.0F);

    EXPECT_EQ(countEnemies(registry), 5u);
}

TEST(MonsterSpawnSystem, SpawnedEntitiesHaveRequiredComponents)
{
    Registry registry;
    std::vector<MovementComponent> patterns{MovementComponent::zigzag(2.0F, 1.0F, 1.0F)};
    std::vector<SpawnEvent> script{makeSpawn(0.1F, 10.0F, 0.0F, 0)};
    MonsterSpawnSystem sys(patterns, script);

    sys.update(registry, 0.3F);

    std::size_t checked = 0;
    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (!registry.isAlive(id))
            continue;
        EXPECT_TRUE(registry.has<TransformComponent>(id));
        EXPECT_TRUE(registry.has<VelocityComponent>(id));
        EXPECT_TRUE(registry.has<MovementComponent>(id));
        EXPECT_TRUE(registry.has<TagComponent>(id));
        EXPECT_TRUE(registry.get<TagComponent>(id).hasTag(EntityTag::Enemy));
        ++checked;
    }
    EXPECT_EQ(checked, countEnemies(registry));
}

TEST(MonsterSpawnSystem, SpawnDistributionUsesAllPatterns)
{
    Registry registry;
    std::vector<MovementComponent> patterns{MovementComponent::linear(1.0F),
                                            MovementComponent::zigzag(2.0F, 1.0F, 1.0F),
                                            MovementComponent::sine(3.0F, 2.0F, 0.5F, 0.1F)};
    std::vector<SpawnEvent> script{makeSpawn(0.01F, 0.0F, 0.0F, 0), makeSpawn(0.02F, 0.0F, 0.2F, 1),
                                   makeSpawn(0.03F, 0.0F, 0.4F, 2)};
    MonsterSpawnSystem sys(patterns, script);

    sys.update(registry, 0.1F);

    bool hasLinear   = false;
    bool hasZigzag   = false;
    bool hasSine     = false;
    std::size_t seen = 0;
    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (!registry.isAlive(id))
            continue;
        ++seen;
        auto& m = registry.get<MovementComponent>(id);
        if (m.pattern == MovementPattern::Linear)
            hasLinear = true;
        if (m.pattern == MovementPattern::Zigzag)
            hasZigzag = true;
        if (m.pattern == MovementPattern::Sine)
            hasSine = true;
    }
    EXPECT_TRUE(hasLinear);
    EXPECT_TRUE(hasZigzag);
    EXPECT_TRUE(hasSine);
    EXPECT_EQ(seen, 3u);
}

TEST(MonsterSpawnSystem, SpawnsAccumulateAcrossUpdates)
{
    Registry registry;
    std::vector<MovementComponent> patterns{MovementComponent::linear(2.0F)};
    std::vector<SpawnEvent> script{makeSpawn(0.05F, 0.0F, 0.0F, 0), makeSpawn(0.15F, 0.0F, 0.5F, 0),
                                   makeSpawn(0.25F, 0.0F, 1.0F, 0)};
    MonsterSpawnSystem sys(patterns, script);

    sys.update(registry, 0.1F);
    EXPECT_EQ(countEnemies(registry), 1u);
    sys.update(registry, 0.2F);
    EXPECT_EQ(countEnemies(registry), 3u);
}
