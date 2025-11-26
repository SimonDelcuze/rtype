#include "state/DirtyTracker.hpp"

#include <gtest/gtest.h>

static TransformComponent makeTransform(float x, float y, float rot = 0.0F, float sx = 1.0F, float sy = 1.0F)
{
    TransformComponent t;
    t.x        = x;
    t.y        = y;
    t.rotation = rot;
    t.scaleX   = sx;
    t.scaleY   = sy;
    return t;
}

static bool hasEntry(const std::vector<DirtyEntry>& entries, EntityId id, DirtyFlag flag)
{
    for (const auto& e : entries) {
        if (e.id == id && hasFlag(e.flags, flag))
            return true;
    }
    return false;
}

TEST(DirtyTracker, MarksSpawn)
{
    DirtyTracker tracker;
    tracker.onSpawn(1, makeTransform(1.0F, 2.0F));
    auto list = tracker.consume();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_TRUE(hasFlag(list[0].flags, DirtyFlag::Spawned));
    EXPECT_FLOAT_EQ(list[0].transform.x, 1.0F);
    EXPECT_FLOAT_EQ(list[0].transform.y, 2.0F);
}

TEST(DirtyTracker, MarksDestroy)
{
    DirtyTracker tracker;
    tracker.onDestroy(5);
    auto list = tracker.consume();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].id, 5u);
    EXPECT_TRUE(hasFlag(list[0].flags, DirtyFlag::Destroyed));
}

TEST(DirtyTracker, DetectsMovement)
{
    DirtyTracker tracker;
    tracker.onSpawn(2, makeTransform(0.0F, 0.0F));
    tracker.trackTransform(2, makeTransform(0.0F, 0.0F));
    tracker.trackTransform(2, makeTransform(1.0F, 0.0F));
    auto list = tracker.consume();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_TRUE(hasEntry(list, 2, DirtyFlag::Spawned));
    EXPECT_TRUE(hasEntry(list, 2, DirtyFlag::Moved));
}

TEST(DirtyTracker, TinyMovementIgnored)
{
    DirtyTracker tracker;
    tracker.onSpawn(3, makeTransform(0.0F, 0.0F));
    tracker.trackTransform(3, makeTransform(1e-5F, 0.0F));
    auto list = tracker.consume();
    EXPECT_FALSE(hasEntry(list, 3, DirtyFlag::Moved));
}

TEST(DirtyTracker, ConsumeClearsFlags)
{
    DirtyTracker tracker;
    tracker.onSpawn(4, makeTransform(0.0F, 0.0F));
    tracker.consume();
    tracker.consume();
    auto list = tracker.consume();
    EXPECT_TRUE(list.empty());
}

TEST(DirtyTracker, StoresLatestTransformOnMove)
{
    DirtyTracker tracker;
    tracker.onSpawn(6, makeTransform(0.0F, 0.0F));
    tracker.trackTransform(6, makeTransform(2.0F, 3.0F, 0.5F, 1.0F, 1.0F));
    auto list  = tracker.consume();
    bool found = false;
    for (const auto& e : list) {
        if (e.id == 6 && hasFlag(e.flags, DirtyFlag::Moved)) {
            found = true;
            EXPECT_FLOAT_EQ(e.transform.x, 2.0F);
            EXPECT_FLOAT_EQ(e.transform.y, 3.0F);
            EXPECT_FLOAT_EQ(e.transform.rotation, 0.5F);
        }
    }
    EXPECT_TRUE(found);
}

TEST(DirtyTracker, DestroyAfterMoveKeepsDestroyFlag)
{
    DirtyTracker tracker;
    tracker.onSpawn(7, makeTransform(0.0F, 0.0F));
    tracker.trackTransform(7, makeTransform(1.0F, 0.0F));
    tracker.onDestroy(7);
    auto list      = tracker.consume();
    bool destroyed = false;
    for (const auto& e : list) {
        if (e.id == 7) {
            destroyed = hasFlag(e.flags, DirtyFlag::Destroyed);
        }
    }
    EXPECT_TRUE(destroyed);
}

TEST(DirtyTracker, MoveAfterSpawnSameTickAggregatesFlags)
{
    DirtyTracker tracker;
    tracker.onSpawn(8, makeTransform(0.0F, 0.0F));
    tracker.trackTransform(8, makeTransform(1.0F, 1.0F));
    auto list     = tracker.consume();
    bool hasSpawn = false;
    bool hasMove  = false;
    for (const auto& e : list) {
        if (e.id == 8) {
            hasSpawn = hasFlag(e.flags, DirtyFlag::Spawned);
            hasMove  = hasFlag(e.flags, DirtyFlag::Moved);
        }
    }
    EXPECT_TRUE(hasSpawn);
    EXPECT_TRUE(hasMove);
}
