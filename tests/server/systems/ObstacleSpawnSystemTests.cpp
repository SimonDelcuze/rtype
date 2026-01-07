#include "levels/ObstacleLibrary.hpp"
#include "systems/ObstacleSpawnSystem.hpp"

#include <cmath>
#include <gtest/gtest.h>

TEST(ObstacleSpawnSystem, SpawnsAnchoredObstacles)
{
    Registry registry;
    HitboxComponent hitbox = HitboxComponent::create(20.0F, 20.0F, 0.0F, 5.0F, true);
    std::vector<ObstacleSpawn> spawns{
        Obstacles::at(0.05F, 100.0F, 200.0F, hitbox, 10),
        Obstacles::top(0.06F, 150.0F, hitbox, 10, 3.0F),
        Obstacles::bottom(0.07F, 200.0F, hitbox, 10, 4.0F),
    };
    ObstacleSpawnSystem system(spawns, 720.0F);

    system.update(registry, 0.1F);

    int count = 0;
    for (EntityId id : registry.view<TransformComponent, TagComponent>()) {
        if (!registry.isAlive(id))
            continue;
        ++count;
        const auto& tag = registry.get<TagComponent>(id);
        EXPECT_TRUE(tag.hasTag(EntityTag::Obstacle));
        EXPECT_TRUE(registry.has<HealthComponent>(id));
        EXPECT_TRUE(registry.has<RenderTypeComponent>(id));
        EXPECT_TRUE(registry.has<VelocityComponent>(id));
        EXPECT_EQ(registry.get<RenderTypeComponent>(id).typeId, 9);
        const auto& vel = registry.get<VelocityComponent>(id);
        EXPECT_FLOAT_EQ(vel.vx, -50.0F);
        EXPECT_FLOAT_EQ(vel.vy, 0.0F);
        const auto& t = registry.get<TransformComponent>(id);
        if (std::abs(t.x - 100.0F) < 0.01F) {
            EXPECT_FLOAT_EQ(t.y, 200.0F);
        } else if (std::abs(t.x - 150.0F) < 0.01F) {
            EXPECT_FLOAT_EQ(t.y, -2.0F);
        } else if (std::abs(t.x - 200.0F) < 0.01F) {
            EXPECT_FLOAT_EQ(t.y, 691.0F);
        }
    }
    EXPECT_EQ(count, 3);
}
