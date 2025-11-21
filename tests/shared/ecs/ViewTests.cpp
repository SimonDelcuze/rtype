#include "components/MovementComponent.hpp"
#include "ecs/Registry.hpp"
#include "ecs/View.hpp"

#include <gtest/gtest.h>

struct Position
{
    float x    = 0.0F;
    float y    = 0.0F;
    Position() = default;
    Position(float x, float y) : x(x), y(y) {}
};

struct Velocity
{
    float dx   = 0.0F;
    float dy   = 0.0F;
    Velocity() = default;
    Velocity(float dx, float dy) : dx(dx), dy(dy) {}
};

struct Health
{
    int hp   = 0;
    Health() = default;
    explicit Health(int hp) : hp(hp) {}
};

TEST(ViewTests, EmptyViewOnEmptyRegistry)
{
    Registry registry;
    auto view = registry.view<Position>();
    EXPECT_EQ(view.begin(), view.end());
}

TEST(ViewTests, EmptyViewWhenNoEntitiesMatch)
{
    Registry registry;
    EntityId e1 = registry.createEntity();
    registry.emplace<Position>(e1, 10.0F, 20.0F);

    auto view = registry.view<Velocity>();
    EXPECT_EQ(view.begin(), view.end());
}

TEST(ViewTests, SingleComponentView)
{
    Registry registry;
    EntityId e1 = registry.createEntity();
    EntityId e2 = registry.createEntity();
    EntityId e3 = registry.createEntity();

    registry.emplace<Position>(e1, 1.0F, 2.0F);
    registry.emplace<Position>(e2, 3.0F, 4.0F);
    registry.emplace<Velocity>(e3, 5.0F, 6.0F);

    std::vector<EntityId> matched;
    for (EntityId id : registry.view<Position>()) {
        matched.push_back(id);
    }

    ASSERT_EQ(matched.size(), 2);
    EXPECT_EQ(matched[0], e1);
    EXPECT_EQ(matched[1], e2);
}

TEST(ViewTests, MultiComponentView)
{
    Registry registry;
    EntityId e1 = registry.createEntity();
    EntityId e2 = registry.createEntity();
    EntityId e3 = registry.createEntity();

    registry.emplace<Position>(e1, 1.0F, 2.0F);
    registry.emplace<Velocity>(e1, 3.0F, 4.0F);

    registry.emplace<Position>(e2, 5.0F, 6.0F);

    registry.emplace<Position>(e3, 7.0F, 8.0F);
    registry.emplace<Velocity>(e3, 9.0F, 10.0F);

    std::vector<EntityId> matched;
    for (EntityId id : registry.view<Position, Velocity>()) {
        matched.push_back(id);
    }

    ASSERT_EQ(matched.size(), 2);
    EXPECT_EQ(matched[0], e1);
    EXPECT_EQ(matched[1], e3);
}

TEST(ViewTests, ThreeComponentView)
{
    Registry registry;
    EntityId e1 = registry.createEntity();
    EntityId e2 = registry.createEntity();
    EntityId e3 = registry.createEntity();

    registry.emplace<Position>(e1, 1.0F, 2.0F);
    registry.emplace<Velocity>(e1, 3.0F, 4.0F);
    registry.emplace<Health>(e1, 100);

    registry.emplace<Position>(e2, 5.0F, 6.0F);
    registry.emplace<Velocity>(e2, 7.0F, 8.0F);

    registry.emplace<Position>(e3, 9.0F, 10.0F);
    registry.emplace<Health>(e3, 50);

    std::vector<EntityId> matched;
    for (EntityId id : registry.view<Position, Velocity, Health>()) {
        matched.push_back(id);
    }

    ASSERT_EQ(matched.size(), 1);
    EXPECT_EQ(matched[0], e1);
}

TEST(ViewTests, IteratorIncrement)
{
    Registry registry;
    EntityId e1 = registry.createEntity();
    EntityId e2 = registry.createEntity();

    registry.emplace<Position>(e1, 1.0F, 2.0F);
    registry.emplace<Position>(e2, 3.0F, 4.0F);

    auto view = registry.view<Position>();
    auto it   = view.begin();

    EXPECT_EQ(*it, e1);
    ++it;
    EXPECT_EQ(*it, e2);
    ++it;
    EXPECT_EQ(it, view.end());
}

TEST(ViewTests, IteratorPostIncrement)
{
    Registry registry;
    EntityId e1 = registry.createEntity();
    EntityId e2 = registry.createEntity();

    registry.emplace<Position>(e1, 1.0F, 2.0F);
    registry.emplace<Position>(e2, 3.0F, 4.0F);

    auto view = registry.view<Position>();
    auto it   = view.begin();

    auto old = it++;
    EXPECT_EQ(*old, e1);
    EXPECT_EQ(*it, e2);
}

TEST(ViewTests, IteratorEquality)
{
    Registry registry;
    auto view = registry.view<Position>();

    auto it1 = view.begin();
    auto it2 = view.begin();

    EXPECT_TRUE(it1 == it2);
    EXPECT_FALSE(it1 != it2);
}

TEST(ViewTests, SkipsDeadEntities)
{
    Registry registry;
    EntityId e1 = registry.createEntity();
    EntityId e2 = registry.createEntity();
    EntityId e3 = registry.createEntity();

    registry.emplace<Position>(e1, 1.0F, 2.0F);
    registry.emplace<Position>(e2, 3.0F, 4.0F);
    registry.emplace<Position>(e3, 5.0F, 6.0F);

    registry.destroyEntity(e2);

    std::vector<EntityId> matched;
    for (EntityId id : registry.view<Position>()) {
        matched.push_back(id);
    }

    ASSERT_EQ(matched.size(), 2);
    EXPECT_EQ(matched[0], e1);
    EXPECT_EQ(matched[1], e3);
}

TEST(ViewTests, ComponentAccessInLoop)
{
    Registry registry;
    EntityId e1 = registry.createEntity();
    EntityId e2 = registry.createEntity();

    registry.emplace<Position>(e1, 10.0F, 20.0F);
    registry.emplace<Velocity>(e1, 1.0F, 2.0F);

    registry.emplace<Position>(e2, 30.0F, 40.0F);
    registry.emplace<Velocity>(e2, 3.0F, 4.0F);

    for (EntityId id : registry.view<Position, Velocity>()) {
        auto& pos = registry.get<Position>(id);
        auto& vel = registry.get<Velocity>(id);
        pos.x += vel.dx;
        pos.y += vel.dy;
    }

    EXPECT_FLOAT_EQ(registry.get<Position>(e1).x, 11.0F);
    EXPECT_FLOAT_EQ(registry.get<Position>(e1).y, 22.0F);
    EXPECT_FLOAT_EQ(registry.get<Position>(e2).x, 33.0F);
    EXPECT_FLOAT_EQ(registry.get<Position>(e2).y, 44.0F);
}

TEST(ViewTests, MultipleViewsSimultaneously)
{
    Registry registry;
    EntityId e1 = registry.createEntity();
    EntityId e2 = registry.createEntity();

    registry.emplace<Position>(e1, 1.0F, 2.0F);
    registry.emplace<Velocity>(e1, 3.0F, 4.0F);

    registry.emplace<Position>(e2, 5.0F, 6.0F);
    registry.emplace<Health>(e2, 100);

    auto viewPosVel    = registry.view<Position, Velocity>();
    auto viewPosHealth = registry.view<Position, Health>();

    std::vector<EntityId> matchedPosVel;
    for (EntityId id : viewPosVel) {
        matchedPosVel.push_back(id);
    }

    std::vector<EntityId> matchedPosHealth;
    for (EntityId id : viewPosHealth) {
        matchedPosHealth.push_back(id);
    }

    ASSERT_EQ(matchedPosVel.size(), 1);
    EXPECT_EQ(matchedPosVel[0], e1);

    ASSERT_EQ(matchedPosHealth.size(), 1);
    EXPECT_EQ(matchedPosHealth[0], e2);
}
