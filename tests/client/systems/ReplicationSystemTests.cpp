#include "components/HealthComponent.hpp"
#include "components/InterpolationComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "network/SnapshotParser.hpp"
#include "systems/ReplicationSystem.hpp"

#include <gtest/gtest.h>
#include <vector>

template <typename... Components> static std::size_t countView(Registry& registry)
{
    std::size_t n = 0;
    for (auto id : registry.view<Components...>()) {
        (void) id;
        ++n;
    }
    return n;
}

static SnapshotParseResult makeSnapshot(std::uint32_t tick, std::uint32_t entityId, float x, float y, float vx,
                                        float vy, std::int16_t hp, bool dead = false)
{
    SnapshotEntity e{};
    e.entityId   = entityId;
    e.updateMask = 0x1F | (dead ? (1u << 8) : 0u) | (1u << 5);
    e.posX       = x;
    e.posY       = y;
    e.velX       = vx;
    e.velY       = vy;
    e.health     = hp;
    if (dead) {
        e.dead = true;
    }

    SnapshotParseResult res{};
    res.header.tickId = tick;
    res.entities.push_back(e);
    return res;
}

static SnapshotParseResult makeCustomSnapshot(std::uint32_t tick, const std::vector<SnapshotEntity>& entities)
{
    SnapshotParseResult res{};
    res.header.tickId = tick;
    res.entities      = entities;
    return res;
}

TEST(ReplicationSystem, NoSnapshotLeavesRegistryEmpty)
{
    ThreadSafeQueue<SnapshotParseResult> queue;
    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);
    EXPECT_EQ(registry.entityCount(), 0u);
}

TEST(ReplicationSystem, CreatesAndUpdatesEntity)
{
    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeSnapshot(1, 10, 5.0F, 6.0F, 1.0F, 2.0F, 50));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    int entitiesWithTransform = 0;
    for (auto id : registry.view<TransformComponent>()) {
        auto& t = registry.get<TransformComponent>(id);
        EXPECT_FLOAT_EQ(t.x, 5.0F);
        EXPECT_FLOAT_EQ(t.y, 6.0F);
        ++entitiesWithTransform;
    }
    EXPECT_EQ(entitiesWithTransform, 1);

    int entitiesWithVel = 0;
    for (auto id : registry.view<VelocityComponent>()) {
        auto& v = registry.get<VelocityComponent>(id);
        EXPECT_FLOAT_EQ(v.vx, 1.0F);
        EXPECT_FLOAT_EQ(v.vy, 2.0F);
        ++entitiesWithVel;
    }
    EXPECT_EQ(entitiesWithVel, 1);

    int entitiesWithHealth = 0;
    for (auto id : registry.view<HealthComponent>()) {
        auto& h = registry.get<HealthComponent>(id);
        EXPECT_EQ(h.current, 50);
        ++entitiesWithHealth;
    }
    EXPECT_EQ(entitiesWithHealth, 1);

    int entitiesWithInterp = 0;
    for (auto id : registry.view<InterpolationComponent>()) {
        auto& interp = registry.get<InterpolationComponent>(id);
        EXPECT_FLOAT_EQ(interp.targetX, 5.0F);
        EXPECT_FLOAT_EQ(interp.targetY, 6.0F);
        EXPECT_FLOAT_EQ(interp.velocityX, 1.0F);
        EXPECT_FLOAT_EQ(interp.velocityY, 2.0F);
        EXPECT_FLOAT_EQ(interp.elapsedTime, 0.0F);
        ++entitiesWithInterp;
    }
    EXPECT_EQ(entitiesWithInterp, 1);
}

TEST(ReplicationSystem, DestroysWhenDeadFlag)
{
    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeSnapshot(1, 20, 0.0F, 0.0F, 0.0F, 0.0F, 10, true));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    int aliveCount = 0;
    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (registry.isAlive(id)) {
            ++aliveCount;
        }
    }
    EXPECT_EQ(aliveCount, 0);
}

TEST(ReplicationSystem, MultipleEntitiesCreated)
{
    SnapshotEntity a{};
    a.entityId   = 1;
    a.updateMask = 0x06;
    a.posX       = 1.0F;
    a.posY       = 2.0F;

    SnapshotEntity b{};
    b.entityId   = 2;
    b.updateMask = 0x18;
    b.velX       = 3.0F;
    b.velY       = 4.0F;

    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeCustomSnapshot(1, {a, b}));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    EXPECT_EQ(registry.entityCount(), 2u);
    EXPECT_EQ(countView<TransformComponent>(registry), 1u);
    EXPECT_EQ(countView<VelocityComponent>(registry), 1u);
}

TEST(ReplicationSystem, VelocityOnlyDoesNotCreateTransform)
{
    SnapshotEntity e{};
    e.entityId   = 5;
    e.updateMask = 0x18;
    e.velX       = 7.0F;
    e.velY       = 8.0F;

    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeCustomSnapshot(1, {e}));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    EXPECT_EQ(countView<VelocityComponent>(registry), 1u);
    EXPECT_EQ(countView<TransformComponent>(registry), 0u);
    EXPECT_EQ(countView<InterpolationComponent>(registry), 0u);
}

TEST(ReplicationSystem, TransformOnlyNoVelocity)
{
    SnapshotEntity e{};
    e.entityId   = 6;
    e.updateMask = 0x06;
    e.posX       = 9.0F;
    e.posY       = -1.0F;

    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeCustomSnapshot(1, {e}));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    EXPECT_EQ(countView<TransformComponent>(registry), 1u);
    EXPECT_EQ(countView<VelocityComponent>(registry), 0u);
    EXPECT_EQ(countView<InterpolationComponent>(registry), 1u);
}

TEST(ReplicationSystem, UpdatesExistingEntityAndPreservesMaxHealth)
{
    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeSnapshot(1, 30, 1.0F, 2.0F, 0.0F, 0.0F, 10));
    queue.push(makeSnapshot(2, 30, 3.0F, 4.0F, 5.0F, 6.0F, 8));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    int count = 0;
    for (auto id : registry.view<TransformComponent, VelocityComponent, HealthComponent>()) {
        auto& t = registry.get<TransformComponent>(id);
        auto& v = registry.get<VelocityComponent>(id);
        auto& h = registry.get<HealthComponent>(id);
        EXPECT_FLOAT_EQ(t.x, 3.0F);
        EXPECT_FLOAT_EQ(t.y, 4.0F);
        EXPECT_FLOAT_EQ(v.vx, 5.0F);
        EXPECT_FLOAT_EQ(v.vy, 6.0F);
        EXPECT_EQ(h.current, 8);
        EXPECT_GE(h.max, 10);
        ++count;
    }
    EXPECT_EQ(count, 1);
}

TEST(ReplicationSystem, HealthDoesNotLowerMax)
{
    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeSnapshot(1, 31, 0.0F, 0.0F, 0.0F, 0.0F, 50));
    queue.push(makeSnapshot(2, 31, 0.0F, 0.0F, 0.0F, 0.0F, 10));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    auto id = registry.view<HealthComponent>().begin().operator*();
    auto& h = registry.get<HealthComponent>(id);
    EXPECT_EQ(h.current, 10);
    EXPECT_EQ(h.max, 50);
}

TEST(ReplicationSystem, ResetsInterpolationOnNewSnapshot)
{
    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeSnapshot(1, 40, 0.0F, 0.0F, 0.0F, 0.0F, 5));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    auto id            = registry.view<InterpolationComponent>().begin().operator*();
    auto& interp       = registry.get<InterpolationComponent>(id);
    interp.elapsedTime = 0.5F;

    queue.push(makeSnapshot(2, 40, 10.0F, 0.0F, 1.0F, 0.0F, 5));
    sys.update(registry, 0.0F);

    EXPECT_FLOAT_EQ(interp.previousX, 0.0F);
    EXPECT_FLOAT_EQ(interp.targetX, 10.0F);
    EXPECT_FLOAT_EQ(interp.elapsedTime, 0.0F);
    EXPECT_FLOAT_EQ(interp.velocityX, 1.0F);
    EXPECT_FLOAT_EQ(interp.velocityY, 0.0F);
}

TEST(ReplicationSystem, PositionNotOverwrittenWhenMissingFields)
{
    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeSnapshot(1, 50, 2.0F, 3.0F, 0.0F, 0.0F, 5));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    SnapshotEntity e{};
    e.entityId   = 50;
    e.updateMask = 0x18;
    e.velX       = 1.0F;
    queue.push(makeCustomSnapshot(2, {e}));
    sys.update(registry, 0.0F);

    auto id = registry.view<TransformComponent>().begin().operator*();
    auto& t = registry.get<TransformComponent>(id);
    EXPECT_FLOAT_EQ(t.x, 2.0F);
    EXPECT_FLOAT_EQ(t.y, 3.0F);
}

TEST(ReplicationSystem, InterpolationNotCreatedWithoutPosition)
{
    SnapshotEntity e{};
    e.entityId   = 60;
    e.updateMask = 0x18;
    e.velX       = 1.0F;
    e.velY       = 2.0F;

    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeCustomSnapshot(1, {e}));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    EXPECT_EQ(countView<InterpolationComponent>(registry), 0u);
}

TEST(ReplicationSystem, MultipleSnapshotsInQueueAreConsumed)
{
    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeSnapshot(1, 70, 0.0F, 0.0F, 0.0F, 0.0F, 1));
    queue.push(makeSnapshot(2, 71, 1.0F, 1.0F, 0.0F, 0.0F, 1));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    EXPECT_EQ(countView<TransformComponent>(registry), 2u);
}

TEST(ReplicationSystem, ReusesEntityMappingForSameRemoteId)
{
    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeSnapshot(1, 80, 1.0F, 1.0F, 0.0F, 0.0F, 5));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    auto firstId = registry.view<TransformComponent>().begin().operator*();

    queue.push(makeSnapshot(2, 80, 9.0F, 9.0F, 0.0F, 0.0F, 5));
    sys.update(registry, 0.0F);

    EXPECT_EQ(countView<TransformComponent>(registry), 1u);
    auto secondId = registry.view<TransformComponent>().begin().operator*();
    EXPECT_EQ(firstId, secondId);
    auto& t = registry.get<TransformComponent>(secondId);
    EXPECT_FLOAT_EQ(t.x, 9.0F);
    EXPECT_FLOAT_EQ(t.y, 9.0F);
}

TEST(ReplicationSystem, StatusFieldIgnoredButEntityCreated)
{
    SnapshotEntity e{};
    e.entityId      = 90;
    e.updateMask    = 0x46;
    e.posX          = 4.0F;
    e.posY          = 5.0F;
    e.statusEffects = 3;

    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeCustomSnapshot(1, {e}));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    EXPECT_EQ(countView<TransformComponent>(registry), 1u);
}

TEST(ReplicationSystem, DeadFlagDoesNotLeaveComponents)
{
    SnapshotEntity e{};
    e.entityId   = 100;
    e.updateMask = 0x1BF;
    e.posX       = 1.0F;
    e.posY       = 1.0F;
    e.velX       = 0.0F;
    e.velY       = 0.0F;
    e.health     = 1;
    e.dead       = true;

    ThreadSafeQueue<SnapshotParseResult> queue;
    queue.push(makeCustomSnapshot(1, {e}));

    ReplicationSystem sys(queue);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);

    EXPECT_EQ(countView<TransformComponent>(registry), 0u);
    EXPECT_EQ(countView<VelocityComponent>(registry), 0u);
    EXPECT_EQ(countView<HealthComponent>(registry), 0u);
}
