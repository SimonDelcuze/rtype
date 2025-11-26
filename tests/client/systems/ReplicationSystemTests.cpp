#include "components/HealthComponent.hpp"
#include "components/InterpolationComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "network/SnapshotParser.hpp"
#include "systems/ReplicationSystem.hpp"

#include <gtest/gtest.h>

static SnapshotParseResult makeSnapshot(std::uint32_t tick, std::uint32_t entityId, float x, float y, float vx,
                                        float vy, std::int16_t hp, bool dead = false)
{
    SnapshotEntity e{};
    e.entityId   = entityId;
    e.updateMask = 0x1F | (dead ? (1u << 8) : 0u) | (1u << 5); // type bit unused, pos/vel/health, dead optional
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
