#include <gtest/gtest.h>

#include "events/EventBus.hpp"

namespace {
struct DamageEvent
{
    std::uint32_t target;
    int amount;
};

struct A
{
};

struct B
{
};
} // namespace

TEST(EventBus, SimpleEventFlow)
{
    EventBus bus;
    bool called = false;
    int captured = 0;

    bus.subscribe<DamageEvent>([&](const DamageEvent& e) {
        called = true;
        captured = e.amount;
    });

    bus.emit<DamageEvent>(DamageEvent{1u, 10});
    EXPECT_FALSE(called);
    bus.process();
    EXPECT_TRUE(called);
    EXPECT_EQ(captured, 10);
}

TEST(EventBus, EmitDuringProcessIsDeferred)
{
    EventBus bus;
    std::vector<char> order;

    bus.subscribe<A>([&](const A&) { order.push_back('A'); bus.emit<B>(B{}); });
    bus.subscribe<B>([&](const B&) { order.push_back('B'); });

    bus.emit<A>(A{});
    bus.process();
    ASSERT_EQ(order.size(), 1u);
    EXPECT_EQ(order[0], 'A');

    bus.process();
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[1], 'B');
}

TEST(EventBus, SubscriberOrderPreserved)
{
    EventBus bus;
    std::vector<int> seq;

    bus.subscribe<DamageEvent>([&](const DamageEvent&) { seq.push_back(1); });
    bus.subscribe<DamageEvent>([&](const DamageEvent&) { seq.push_back(2); });
    bus.subscribe<DamageEvent>([&](const DamageEvent&) { seq.push_back(3); });

    bus.emit<DamageEvent>(DamageEvent{42u, 1});
    bus.process();

    ASSERT_EQ(seq.size(), 3u);
    EXPECT_EQ(seq[0], 1);
    EXPECT_EQ(seq[1], 2);
    EXPECT_EQ(seq[2], 3);
}

