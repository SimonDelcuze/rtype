#include "input/InputBuffer.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <vector>

TEST(InputBuffer, PushThenTryPopReturnsElement)
{
    InputBuffer buf;
    InputCommand cmd{.flags = 1, .sequenceId = 42, .posX = 1.0F, .posY = 2.0F, .angle = 0.5F};
    buf.push(cmd);
    InputCommand out{};
    ASSERT_TRUE(buf.tryPop(out));
    EXPECT_EQ(out.flags, 1);
    EXPECT_EQ(out.sequenceId, 42u);
    EXPECT_FLOAT_EQ(out.posX, 1.0F);
    EXPECT_FLOAT_EQ(out.posY, 2.0F);
    EXPECT_FLOAT_EQ(out.angle, 0.5F);
}

TEST(InputBuffer, TryPopOnEmptyReturnsFalse)
{
    InputBuffer buf;
    InputCommand out{};
    EXPECT_FALSE(buf.tryPop(out));
}

TEST(InputBuffer, PopReturnsOptionalWithData)
{
    InputBuffer buf;
    buf.push(InputCommand{.flags = 3, .sequenceId = 7});
    auto res = buf.pop();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->flags, 3);
    EXPECT_EQ(res->sequenceId, 7u);
}

TEST(InputBuffer, PopReturnsNulloptWhenEmpty)
{
    InputBuffer buf;
    auto res = buf.pop();
    EXPECT_FALSE(res.has_value());
}

TEST(InputBuffer, PreservesOrderMultipleElements)
{
    InputBuffer buf;
    for (std::uint32_t i = 0; i < 5; ++i) {
        buf.push(InputCommand{.flags = static_cast<std::uint16_t>(i), .sequenceId = i});
    }
    for (std::uint32_t i = 0; i < 5; ++i) {
        InputCommand out{};
        ASSERT_TRUE(buf.tryPop(out));
        EXPECT_EQ(out.sequenceId, i);
        EXPECT_EQ(out.flags, i);
    }
}

TEST(InputBuffer, ThreadSafetyPushFromMultipleThreads)
{
    InputBuffer buf;
    const int threads   = 4;
    const int perThread = 10;
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&buf, t, perThread]() {
            for (int i = 0; i < perThread; ++i) {
                buf.push(InputCommand{.flags      = static_cast<std::uint16_t>(t),
                                      .sequenceId = static_cast<std::uint32_t>(t * perThread + i)});
            }
        });
    }
    for (auto& th : workers) {
        th.join();
    }
    int count = 0;
    InputCommand out{};
    while (buf.tryPop(out)) {
        ++count;
    }
    EXPECT_EQ(count, threads * perThread);
}

TEST(InputBuffer, TryPopDrainsQueue)
{
    InputBuffer buf;
    buf.push(InputCommand{.sequenceId = 1});
    buf.push(InputCommand{.sequenceId = 2});
    InputCommand out{};
    ASSERT_TRUE(buf.tryPop(out));
    ASSERT_TRUE(buf.tryPop(out));
    EXPECT_FALSE(buf.tryPop(out));
}

TEST(InputBuffer, PopDrainsQueue)
{
    InputBuffer buf;
    buf.push(InputCommand{.sequenceId = 10});
    buf.push(InputCommand{.sequenceId = 11});
    auto a = buf.pop();
    auto b = buf.pop();
    auto c = buf.pop();
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_FALSE(c.has_value());
}

TEST(InputBuffer, CanHandleLargeNumberOfCommands)
{
    InputBuffer buf;
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        buf.push(InputCommand{.sequenceId = static_cast<std::uint32_t>(i)});
    }
    int popped = 0;
    InputCommand out{};
    while (buf.tryPop(out)) {
        ++popped;
    }
    EXPECT_EQ(popped, count);
}

TEST(InputBuffer, LastPoppedMatchesLastPushed)
{
    InputBuffer buf;
    buf.push(InputCommand{.sequenceId = 1});
    buf.push(InputCommand{.sequenceId = 2});
    buf.push(InputCommand{.sequenceId = 3});
    InputCommand out{};
    while (buf.tryPop(out)) {
    }
    EXPECT_EQ(out.sequenceId, 3u);
}
