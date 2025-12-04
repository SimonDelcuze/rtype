#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/CameraSystem.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <gtest/gtest.h>
#include <limits>

class CameraSystemTests : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        window = std::make_unique<sf::RenderWindow>(sf::VideoMode({800u, 600u}), "Test", sf::Style::None);
        if (!window->isOpen()) {
            GTEST_SKIP() << "RenderWindow not available (no X11/display)";
        }
    }

    void TearDown() override
    {
        window.reset();
    }

    std::unique_ptr<sf::RenderWindow> window;
};

TEST_F(CameraSystemTests, UpdateWithNoCamera)
{
    Registry registry;
    CameraSystem system(*window);

    EXPECT_NO_THROW(system.update(registry, 0.016F));
    EXPECT_EQ(system.getActiveCamera(), std::numeric_limits<EntityId>::max());
}

TEST_F(CameraSystemTests, FindsActiveCamera)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId cameraEntity = registry.createEntity();
    registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(100.0F, 200.0F));

    system.update(registry, 0.016F);

    EXPECT_EQ(system.getActiveCamera(), cameraEntity);
}

TEST_F(CameraSystemTests, IgnoresInactiveCamera)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId cameraEntity = registry.createEntity();
    auto& camera          = registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(100.0F, 200.0F));
    camera.active         = false;

    system.update(registry, 0.016F);

    EXPECT_EQ(system.getActiveCamera(), std::numeric_limits<EntityId>::max());
}

TEST_F(CameraSystemTests, PrefersFirstActiveCamera)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId camera1 = registry.createEntity();
    EntityId camera2 = registry.createEntity();

    registry.emplace<CameraComponent>(camera1, CameraComponent::create(100.0F, 200.0F));
    registry.emplace<CameraComponent>(camera2, CameraComponent::create(300.0F, 400.0F));

    system.update(registry, 0.016F);

    EntityId activeCamera = system.getActiveCamera();
    EXPECT_TRUE(activeCamera == camera1 || activeCamera == camera2);
}

TEST_F(CameraSystemTests, AppliesCameraPosition)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId cameraEntity = registry.createEntity();
    registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(400.0F, 300.0F));

    system.update(registry, 0.016F);

    const sf::View& view = system.getView();
    EXPECT_FLOAT_EQ(view.getCenter().x, 400.0F);
    EXPECT_FLOAT_EQ(view.getCenter().y, 300.0F);
}

TEST_F(CameraSystemTests, AppliesCameraZoom)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId cameraEntity = registry.createEntity();
    auto& camera          = registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(0.0F, 0.0F, 2.0F));

    system.update(registry, 0.016F);

    const sf::View& view = system.getView();
    EXPECT_LT(view.getSize().x, 800.0F);
    EXPECT_LT(view.getSize().y, 600.0F);
}

TEST_F(CameraSystemTests, AppliesCameraOffset)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId cameraEntity = registry.createEntity();
    auto& camera          = registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(100.0F, 100.0F));
    camera.setOffset(50.0F, 25.0F);

    system.update(registry, 0.016F);

    const sf::View& view = system.getView();
    EXPECT_FLOAT_EQ(view.getCenter().x, 150.0F);
    EXPECT_FLOAT_EQ(view.getCenter().y, 125.0F);
}

TEST_F(CameraSystemTests, AppliesCameraRotation)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId cameraEntity = registry.createEntity();
    auto& camera          = registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(0.0F, 0.0F));
    camera.setRotation(45.0F);

    system.update(registry, 0.016F);

    const sf::View& view = system.getView();
    EXPECT_FLOAT_EQ(view.getRotation().asDegrees(), 45.0F);
}

TEST_F(CameraSystemTests, WorldBoundsClampingDisabledByDefault)
{
    Registry registry;
    CameraSystem system(*window);

    system.setWorldBounds(0.0F, 0.0F, 1000.0F, 1000.0F);

    EntityId cameraEntity = registry.createEntity();
    auto& camera          = registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(-500.0F, -500.0F));

    system.update(registry, 0.016F);

    EXPECT_FLOAT_EQ(camera.x, -500.0F);
    EXPECT_FLOAT_EQ(camera.y, -500.0F);
}

TEST_F(CameraSystemTests, WorldBoundsClampingEnabled)
{
    Registry registry;
    CameraSystem system(*window);

    system.setWorldBounds(0.0F, 0.0F, 1000.0F, 1000.0F);
    system.setWorldBoundsEnabled(true);

    EntityId cameraEntity = registry.createEntity();
    auto& camera          = registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(-500.0F, -500.0F));

    system.update(registry, 0.016F);

    EXPECT_GE(camera.x, 0.0F);
    EXPECT_GE(camera.y, 0.0F);
    EXPECT_LE(camera.x, 1000.0F);
    EXPECT_LE(camera.y, 1000.0F);
}

TEST_F(CameraSystemTests, GetViewReturnsReference)
{
    CameraSystem system(*window);

    sf::View& view = system.getView();
    view.setCenter(sf::Vector2f(100.0F, 200.0F));

    EXPECT_FLOAT_EQ(system.getView().getCenter().x, 100.0F);
    EXPECT_FLOAT_EQ(system.getView().getCenter().y, 200.0F);
}

TEST_F(CameraSystemTests, SkipsDeadEntities)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId cameraEntity = registry.createEntity();
    registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(100.0F, 200.0F));
    registry.destroyEntity(cameraEntity);

    system.update(registry, 0.016F);

    EXPECT_EQ(system.getActiveCamera(), std::numeric_limits<EntityId>::max());
}

TEST_F(CameraSystemTests, FollowsTargetEntity)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId target = registry.createEntity();
    registry.emplace<TransformComponent>(target, TransformComponent::create(500.0F, 300.0F));

    EntityId cameraEntity = registry.createEntity();
    auto& camera          = registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(0.0F, 0.0F));
    camera.setTarget(target, 10.0F);

    system.update(registry, 0.016F);

    EXPECT_GT(camera.x, 0.0F);
    EXPECT_GT(camera.y, 0.0F);
}

TEST_F(CameraSystemTests, SmoothFollowConvergesToTarget)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId target = registry.createEntity();
    registry.emplace<TransformComponent>(target, TransformComponent::create(100.0F, 100.0F));

    EntityId cameraEntity = registry.createEntity();
    auto& camera          = registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(0.0F, 0.0F));
    camera.setTarget(target, 5.0F);

    for (int i = 0; i < 100; ++i) {
        system.update(registry, 0.016F);
    }

    EXPECT_NEAR(camera.x, 100.0F, 1.0F);
    EXPECT_NEAR(camera.y, 100.0F, 1.0F);
}

TEST_F(CameraSystemTests, FollowDisabledWhenTargetDies)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId target = registry.createEntity();
    registry.emplace<TransformComponent>(target, TransformComponent::create(100.0F, 100.0F));

    EntityId cameraEntity = registry.createEntity();
    auto& camera          = registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(0.0F, 0.0F));
    camera.setTarget(target);

    system.update(registry, 0.016F);
    EXPECT_TRUE(camera.followEnabled);

    registry.destroyEntity(target);
    system.update(registry, 0.016F);

    EXPECT_FALSE(camera.followEnabled);
}

TEST_F(CameraSystemTests, ClearTargetStopsFollow)
{
    Registry registry;
    CameraSystem system(*window);

    EntityId target = registry.createEntity();
    registry.emplace<TransformComponent>(target, TransformComponent::create(100.0F, 100.0F));

    EntityId cameraEntity = registry.createEntity();
    auto& camera          = registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(0.0F, 0.0F));
    camera.setTarget(target);

    EXPECT_TRUE(camera.followEnabled);

    camera.clearTarget();

    EXPECT_FALSE(camera.followEnabled);
    EXPECT_EQ(camera.targetEntity, std::numeric_limits<EntityId>::max());
}

TEST_F(CameraSystemTests, FollowWithWorldBounds)
{
    Registry registry;
    CameraSystem system(*window);

    system.setWorldBounds(0.0F, 0.0F, 500.0F, 500.0F);
    system.setWorldBoundsEnabled(true);

    EntityId target = registry.createEntity();
    registry.emplace<TransformComponent>(target, TransformComponent::create(1000.0F, 1000.0F));

    EntityId cameraEntity = registry.createEntity();
    auto& camera          = registry.emplace<CameraComponent>(cameraEntity, CameraComponent::create(250.0F, 250.0F));
    camera.setTarget(target, 10.0F);

    for (int i = 0; i < 100; ++i) {
        system.update(registry, 0.016F);
    }

    EXPECT_LE(camera.x, 500.0F);
    EXPECT_LE(camera.y, 500.0F);
    EXPECT_GE(camera.x, 0.0F);
    EXPECT_GE(camera.y, 0.0F);
}
