#include "systems/CollisionSystem.hpp"

#include <gtest/gtest.h>
#include <limits>

static bool containsPair(const std::vector<Collision>& collisions, EntityId a, EntityId b)
{
    for (const auto& c : collisions) {
        if ((c.a == a && c.b == b) || (c.a == b && c.b == a))
            return true;
    }
    return false;
}

TEST(CollisionSystem, DetectsOverlap)
{
    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    registry.emplace<TransformComponent>(a, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TransformComponent>(b, TransformComponent::create(1.0F, 0.0F));
    registry.emplace<HitboxComponent>(a, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));
    registry.emplace<HitboxComponent>(b, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));

    CollisionSystem sys;
    auto col = sys.detect(registry);
    EXPECT_TRUE(containsPair(col, a, b));
}

TEST(CollisionSystem, NoOverlapWhenSeparated)
{
    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    registry.emplace<TransformComponent>(a, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TransformComponent>(b, TransformComponent::create(5.0F, 0.0F));
    registry.emplace<HitboxComponent>(a, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));
    registry.emplace<HitboxComponent>(b, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));

    CollisionSystem sys;
    auto col = sys.detect(registry);
    EXPECT_FALSE(containsPair(col, a, b));
}

TEST(CollisionSystem, TouchingEdgesCountsAsOverlap)
{
    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    registry.emplace<TransformComponent>(a, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TransformComponent>(b, TransformComponent::create(2.0F, 0.0F));
    registry.emplace<HitboxComponent>(a, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));
    registry.emplace<HitboxComponent>(b, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));

    CollisionSystem sys;
    auto col = sys.detect(registry);
    EXPECT_TRUE(containsPair(col, a, b));
}

TEST(CollisionSystem, AppliesOffsets)
{
    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    registry.emplace<TransformComponent>(a, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TransformComponent>(b, TransformComponent::create(3.0F, 0.0F));
    registry.emplace<HitboxComponent>(a, HitboxComponent::create(2.0F, 2.0F, 1.0F, 0.0F, true));
    registry.emplace<HitboxComponent>(b, HitboxComponent::create(2.0F, 2.0F, -1.0F, 0.0F, true));

    CollisionSystem sys;
    auto col = sys.detect(registry);
    EXPECT_TRUE(containsPair(col, a, b));
}

TEST(CollisionSystem, IgnoresDeadEntities)
{
    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    registry.emplace<TransformComponent>(a, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TransformComponent>(b, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<HitboxComponent>(a, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));
    registry.emplace<HitboxComponent>(b, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));
    registry.destroyEntity(b);

    CollisionSystem sys;
    auto col = sys.detect(registry);
    EXPECT_FALSE(containsPair(col, a, b));
}

TEST(CollisionSystem, SkipsInvalidDimensions)
{
    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    registry.emplace<TransformComponent>(a, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TransformComponent>(b, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<HitboxComponent>(a, HitboxComponent::create(0.0F, 2.0F, 0.0F, 0.0F, true));
    registry.emplace<HitboxComponent>(b, HitboxComponent::create(2.0F, 0.0F, 0.0F, 0.0F, true));

    CollisionSystem sys;
    auto col = sys.detect(registry);
    EXPECT_TRUE(col.empty());
}

TEST(CollisionSystem, SkipsNonFiniteValues)
{
    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    registry.emplace<TransformComponent>(a, TransformComponent::create(std::numeric_limits<float>::infinity(), 0.0F));
    registry.emplace<TransformComponent>(b, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<HitboxComponent>(a, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));
    registry.emplace<HitboxComponent>(b, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));

    CollisionSystem sys;
    auto col = sys.detect(registry);
    EXPECT_TRUE(col.empty());
}

TEST(CollisionSystem, InactiveHitboxIgnored)
{
    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    registry.emplace<TransformComponent>(a, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TransformComponent>(b, TransformComponent::create(0.0F, 0.0F));
    HitboxComponent ha = HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true);
    HitboxComponent hb = HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, false);
    registry.emplace<HitboxComponent>(a, ha);
    registry.emplace<HitboxComponent>(b, hb);

    CollisionSystem sys;
    auto col = sys.detect(registry);
    EXPECT_TRUE(col.empty());
}

TEST(CollisionSystem, CountsAllPairs)
{
    Registry registry;
    EntityId a = registry.createEntity();
    EntityId b = registry.createEntity();
    EntityId c = registry.createEntity();
    registry.emplace<TransformComponent>(a, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TransformComponent>(b, TransformComponent::create(0.5F, 0.0F));
    registry.emplace<TransformComponent>(c, TransformComponent::create(1.0F, 0.0F));
    registry.emplace<HitboxComponent>(a, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));
    registry.emplace<HitboxComponent>(b, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));
    registry.emplace<HitboxComponent>(c, HitboxComponent::create(2.0F, 2.0F, 0.0F, 0.0F, true));

    CollisionSystem sys;
    auto col = sys.detect(registry);
    EXPECT_EQ(col.size(), 3u);
    EXPECT_TRUE(containsPair(col, a, b));
    EXPECT_TRUE(containsPair(col, a, c));
    EXPECT_TRUE(containsPair(col, b, c));
}

TEST(CollisionSystem, CircleAndBoxOverlap)
{
    Registry registry;
    EntityId circle = registry.createEntity();
    EntityId box    = registry.createEntity();
    registry.emplace<TransformComponent>(circle, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TransformComponent>(box, TransformComponent::create(1.5F, 0.0F));
    registry.emplace<ColliderComponent>(circle, ColliderComponent::circle(2.0F));
    registry.emplace<HitboxComponent>(box, HitboxComponent::create(2.0F, 2.0F));
    CollisionSystem sys;
    auto col = sys.detect(registry);
    EXPECT_TRUE(containsPair(col, circle, box));
}

TEST(CollisionSystem, PolygonSeparatesCorrectly)
{
    Registry registry;
    EntityId poly1 = registry.createEntity();
    EntityId poly2 = registry.createEntity();
    std::vector<std::array<float, 2>> pts{{{0.0F, 0.0F}, {2.0F, 0.0F}, {2.0F, 2.0F}, {0.0F, 2.0F}}};
    registry.emplace<TransformComponent>(poly1, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<TransformComponent>(poly2, TransformComponent::create(5.0F, 0.0F));
    registry.emplace<ColliderComponent>(poly1, ColliderComponent::polygon(pts));
    registry.emplace<ColliderComponent>(poly2, ColliderComponent::polygon(pts));

    CollisionSystem sys;
    auto col = sys.detect(registry);
    EXPECT_TRUE(col.empty());

    registry.get<TransformComponent>(poly2).x = 1.0F;
    col                                       = sys.detect(registry);
    EXPECT_TRUE(containsPair(col, poly1, poly2));
}
