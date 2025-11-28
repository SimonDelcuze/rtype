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

TEST(MonsterSpawnSystem, SpawnsAtInterval)
{
    Registry registry;
    MonsterSpawnConfig cfg{};
    cfg.spawnInterval = 0.5F;
    cfg.spawnX        = 100.0F;
    cfg.yMin          = 0.0F;
    cfg.yMax          = 10.0F;
    std::vector<MovementComponent> patterns{MovementComponent::linear(5.0F)};
    MonsterSpawnSystem sys(cfg, patterns, 42);

    sys.update(registry, 0.4F);
    EXPECT_EQ(countEnemies(registry), 0u);
    sys.update(registry, 0.2F);
    EXPECT_EQ(countEnemies(registry), 1u);
    sys.update(registry, 1.0F);
    EXPECT_EQ(countEnemies(registry), 3u);
}

TEST(MonsterSpawnSystem, SpawnsOnRightSideAndWithinYRange)
{
    Registry registry;
    MonsterSpawnConfig cfg{};
    cfg.spawnInterval = 0.1F;
    cfg.spawnX        = 500.0F;
    cfg.yMin          = -5.0F;
    cfg.yMax          = 5.0F;
    std::vector<MovementComponent> patterns{MovementComponent::linear(2.0F)};
    MonsterSpawnSystem sys(cfg, patterns, 7);

    sys.update(registry, 0.5F);

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
    MonsterSpawnConfig cfg{};
    cfg.spawnInterval = 0.01F;
    cfg.spawnX        = 0.0F;
    cfg.yMin          = 0.0F;
    cfg.yMax          = 1.0F;
    std::vector<MovementComponent> patterns{MovementComponent::linear(3.0F),
                                            MovementComponent::zigzag(4.0F, 1.0F, 2.0F)};
    MonsterSpawnSystem sys(cfg, patterns, 1234);

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
    MonsterSpawnConfig cfg{};
    cfg.spawnInterval = 0.1F;
    cfg.spawnX        = 0.0F;
    cfg.yMin          = 0.0F;
    cfg.yMax          = 1.0F;
    std::vector<MovementComponent> patterns;
    MonsterSpawnSystem sys(cfg, patterns, 1);

    sys.update(registry, 1.0F);

    EXPECT_EQ(countEnemies(registry), 0u);
}

TEST(MonsterSpawnSystem, MultipleSpawnsInSingleTick)
{
    Registry registry;
    MonsterSpawnConfig cfg{};
    cfg.spawnInterval = 0.2F;
    cfg.spawnX        = 50.0F;
    cfg.yMin          = 0.0F;
    cfg.yMax          = 1.0F;
    std::vector<MovementComponent> patterns{MovementComponent::sine(1.0F, 1.0F, 1.0F, 0.0F)};
    MonsterSpawnSystem sys(cfg, patterns, 5);

    sys.update(registry, 1.0F);

    EXPECT_EQ(countEnemies(registry), 5u);
}

TEST(MonsterSpawnSystem, SpawnedEntitiesHaveRequiredComponents)
{
    Registry registry;
    MonsterSpawnConfig cfg{};
    cfg.spawnInterval = 0.1F;
    cfg.spawnX        = 10.0F;
    cfg.yMin          = 0.0F;
    cfg.yMax          = 0.0F;
    std::vector<MovementComponent> patterns{MovementComponent::zigzag(2.0F, 1.0F, 1.0F)};
    MonsterSpawnSystem sys(cfg, patterns, 9);

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

TEST(MonsterSpawnSystem, SpawnDistributionUsesAllPatternsOverTime)
{
    Registry registry;
    MonsterSpawnConfig cfg{};
    cfg.spawnInterval = 0.05F;
    cfg.spawnX        = 0.0F;
    cfg.yMin          = 0.0F;
    cfg.yMax          = 1.0F;
    std::vector<MovementComponent> patterns{MovementComponent::linear(1.0F),
                                            MovementComponent::zigzag(2.0F, 1.0F, 1.0F),
                                            MovementComponent::sine(3.0F, 2.0F, 0.5F, 0.1F)};
    MonsterSpawnSystem sys(cfg, patterns, 42);

    sys.update(registry, 1.0F);

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
    EXPECT_GE(seen, 10u);
}

TEST(MonsterSpawnSystem, SpawnsAccumulateAcrossUpdates)
{
    Registry registry;
    MonsterSpawnConfig cfg{};
    cfg.spawnInterval = 0.2F;
    cfg.spawnX        = 25.0F;
    cfg.yMin          = -1.0F;
    cfg.yMax          = 1.0F;
    std::vector<MovementComponent> patterns{MovementComponent::linear(2.0F)};
    MonsterSpawnSystem sys(cfg, patterns, 3);

    sys.update(registry, 0.15F);
    EXPECT_EQ(countEnemies(registry), 0u);
    sys.update(registry, 0.1F);
    EXPECT_EQ(countEnemies(registry), 1u);
    sys.update(registry, 0.4F);
    EXPECT_EQ(countEnemies(registry), 3u);
}

TEST(MonsterSpawnSystem, RespectsUpperAndLowerYBounds)
{
    Registry registry;
    MonsterSpawnConfig cfg{};
    cfg.spawnInterval = 0.05F;
    cfg.spawnX        = 0.0F;
    cfg.yMin          = -10.0F;
    cfg.yMax          = -5.0F;
    std::vector<MovementComponent> patterns{MovementComponent::linear(1.0F)};
    MonsterSpawnSystem sys(cfg, patterns, 999);

    sys.update(registry, 0.5F);

    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (!registry.isAlive(id))
            continue;
        auto& t = registry.get<TransformComponent>(id);
        EXPECT_GE(t.y, -10.0F);
        EXPECT_LE(t.y, -5.0F);
    }
}
