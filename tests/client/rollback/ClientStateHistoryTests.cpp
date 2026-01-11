#include "rollback/ClientStateHistory.hpp"

#include <gtest/gtest.h>

class ClientStateHistoryTest : public ::testing::Test
{
  protected:
    ClientStateHistory history;
};

TEST_F(ClientStateHistoryTest, InitialState)
{
    EXPECT_TRUE(history.empty());
    EXPECT_EQ(history.size(), 0);
    EXPECT_FALSE(history.getLatest().has_value());
}

TEST_F(ClientStateHistoryTest, AddSnapshot)
{
    std::unordered_map<EntityId, ClientEntityState> entities;
    entities[1] = {10.0f, 20.0f, 1.0f, 0.0f, 100, true};

    history.addSnapshot(100, entities, 12345);

    EXPECT_FALSE(history.empty());
    EXPECT_EQ(history.size(), 1);

    auto latest = history.getLatest();
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->get().tick, 100);
    EXPECT_EQ(latest->get().checksum, 12345);
    EXPECT_EQ(latest->get().entities.at(1).posX, 10.0f);
}

TEST_F(ClientStateHistoryTest, GetSnapshotByTick)
{
    std::unordered_map<EntityId, ClientEntityState> entities;
    history.addSnapshot(100, entities, 111);
    history.addSnapshot(101, entities, 222);
    history.addSnapshot(102, entities, 333);

    auto s101 = history.getSnapshot(101);
    ASSERT_TRUE(s101.has_value());
    EXPECT_EQ(s101->get().checksum, 222);

    auto s99 = history.getSnapshot(99);
    EXPECT_FALSE(s99.has_value());
}

TEST_F(ClientStateHistoryTest, CircularBufferLogic)
{
    std::unordered_map<EntityId, ClientEntityState> entities;

    for (std::uint64_t i = 0; i < ClientStateHistory::HISTORY_SIZE; ++i) {
        history.addSnapshot(i, entities, static_cast<std::uint32_t>(i));
    }

    EXPECT_EQ(history.size(), ClientStateHistory::HISTORY_SIZE);
    EXPECT_TRUE(history.hasSnapshot(0));

    history.addSnapshot(ClientStateHistory::HISTORY_SIZE, entities, 999);

    EXPECT_EQ(history.size(), ClientStateHistory::HISTORY_SIZE);
    EXPECT_FALSE(history.hasSnapshot(0));
    EXPECT_TRUE(history.hasSnapshot(1));
    EXPECT_TRUE(history.hasSnapshot(ClientStateHistory::HISTORY_SIZE));
}

TEST_F(ClientStateHistoryTest, Clear)
{
    std::unordered_map<EntityId, ClientEntityState> entities;
    history.addSnapshot(100, entities, 111);

    history.clear();

    EXPECT_TRUE(history.empty());
    EXPECT_EQ(history.size(), 0);
    EXPECT_FALSE(history.hasSnapshot(100));
}
