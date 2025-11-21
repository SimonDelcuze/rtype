#include "concurrency/ThreadSafeQueue.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <vector>

TEST(ThreadSafeQueueServer, TryPopEmptyReturnsFalse)
{
    ThreadSafeQueue<int> q;
    int value = 0;
    EXPECT_FALSE(q.tryPop(value));
}

TEST(ThreadSafeQueueServer, SpscOrderPreserved)
{
    ThreadSafeQueue<int> q;
    const int count = 1000;
    std::vector<int> consumed;
    consumed.reserve(count);

    std::thread consumer([&] {
        for (int i = 0; i < count; ++i)
            consumed.push_back(q.waitPop());
    });

    for (int i = 0; i < count; ++i)
        q.push(i);

    consumer.join();

    ASSERT_EQ(consumed.size(), static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
        EXPECT_EQ(consumed[i], i);
}
