#include "components/HitboxComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/Window.hpp"
#include "systems/HitboxDebugSystem.hpp"

#include <gtest/gtest.h>

class HitboxDebugSystemTests : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        window = new Window({64u, 64u}, "Test");
        system = new HitboxDebugSystem(*window);
        e = registry.createEntity();
    }

    void TearDown() override
    {
        delete system;
        delete window;
    }

    Window* window;
    HitboxDebugSystem* system;
    Registry registry;
    EntityId e;
};

TEST_F(HitboxDebugSystemTests, UpdateWithNoEntities)
{
    system->setEnabled(true);
    EXPECT_NO_THROW(system->update(registry));
}

TEST_F(HitboxDebugSystemTests, DisabledSystemDoesNotDraw)
{
    EntityId entity = registry.createEntity();
    registry.emplace<HitboxComponent>(entity, HitboxComponent::create(10.0F, 10.0F));
    registry.emplace<TransformComponent>(entity);

    system->setEnabled(false);
    EXPECT_NO_THROW(system->update(registry));
}

TEST_F(HitboxDebugSystemTests, EnabledSystemDrawsHitboxes)
{
    EntityId entity = registry.createEntity();
    registry.emplace<HitboxComponent>(entity, HitboxComponent::create(10.0F, 10.0F));
    registry.emplace<TransformComponent>(entity);

    system->setEnabled(true);
    EXPECT_NO_THROW(system->update(registry));
}

TEST_F(HitboxDebugSystemTests, SkipsInactiveHitboxes)
{
    EntityId entity = registry.createEntity();
    auto& hitbox    = registry.emplace<HitboxComponent>(entity, HitboxComponent::create(10.0F, 10.0F));
    hitbox.isActive = false;
    registry.emplace<TransformComponent>(entity);

    system->setEnabled(true);
    EXPECT_NO_THROW(system->update(registry));
}

TEST_F(HitboxDebugSystemTests, SkipsEntitiesWithoutHitbox)
{
    EntityId entity = registry.createEntity();
    registry.emplace<TransformComponent>(entity);

    system->setEnabled(true);
    EXPECT_NO_THROW(system->update(registry));
}

TEST_F(HitboxDebugSystemTests, SkipsEntitiesWithoutTransform)
{
    EntityId entity = registry.createEntity();
    registry.emplace<HitboxComponent>(entity, HitboxComponent::create(10.0F, 10.0F));

    system->setEnabled(true);
    EXPECT_NO_THROW(system->update(registry));
}

TEST_F(HitboxDebugSystemTests, SkipsDeadEntities)
{
    EntityId entity = registry.createEntity();
    registry.emplace<HitboxComponent>(entity, HitboxComponent::create(10.0F, 10.0F));
    registry.emplace<TransformComponent>(entity);

    registry.destroyEntity(entity);

    system->setEnabled(true);
    EXPECT_NO_THROW(system->update(registry));
}

TEST_F(HitboxDebugSystemTests, HandlesMultipleEntities)
{
    for (int i = 0; i < 5; ++i) {
        EntityId entity = registry.createEntity();
        registry.emplace<HitboxComponent>(entity, HitboxComponent::create(10.0F, 10.0F));
        registry.emplace<TransformComponent>(entity);
    }

    system->setEnabled(true);
    EXPECT_NO_THROW(system->update(registry));
}

TEST_F(HitboxDebugSystemTests, SetColorDoesNotCrash)
{
    system->setColor(sf::Color::Red);
    EXPECT_NO_THROW(system->update(registry));
}

TEST_F(HitboxDebugSystemTests, SetThicknessDoesNotCrash)
{
    system->setThickness(2.0F);
    EXPECT_NO_THROW(system->update(registry));
}

TEST_F(HitboxDebugSystemTests, HitboxWithOffset)
{
    EntityId entity = registry.createEntity();
    registry.emplace<HitboxComponent>(entity, HitboxComponent::create(10.0F, 10.0F, 5.0F, 5.0F));
    registry.emplace<TransformComponent>(entity, TransformComponent::create(100.0F, 100.0F));

    system->setEnabled(true);
    EXPECT_NO_THROW(system->update(registry));
}
