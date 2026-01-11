#include "ecs/Registry.hpp"
#include "network/NetworkMessageHandler.hpp"
#include "systems/NetworkMessageSystem.hpp"

#include <gtest/gtest.h>

TEST(NetworkMessageSystemTest, PollsHandler)
{
    ThreadSafeQueue<std::vector<std::uint8_t>> rawQueue;
    ThreadSafeQueue<SnapshotParseResult> snapshotQueue;
    ThreadSafeQueue<LevelInitData> levelInitQueue;
    ThreadSafeQueue<LevelEventData> levelEventQueue;
    ThreadSafeQueue<EntitySpawnPacket> spawnQueue;
    ThreadSafeQueue<EntityDestroyedPacket> destroyQueue;

    NetworkMessageHandler handler(rawQueue, snapshotQueue, levelInitQueue, levelEventQueue, spawnQueue, destroyQueue);
    NetworkMessageSystem system(handler);
    Registry registry;

    rawQueue.push({0, 0, 0, 0});

    EXPECT_FALSE(rawQueue.empty());
    system.update(registry, 0.16f);
    EXPECT_TRUE(rawQueue.empty());
}
