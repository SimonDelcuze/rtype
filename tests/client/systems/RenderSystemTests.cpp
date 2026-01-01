#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/Window.hpp"
#include "systems/RenderSystem.hpp"
#include "graphics/backends/sfml/SFMLTexture.hpp"

#include <gtest/gtest.h>

TEST(RenderSystem, AppliesTransformToSprite)
{
    Window window{{64u, 64u}, "Test"};
    RenderSystem renderSystem(window);
    Registry registry;

    auto texture = std::make_shared<SFMLTexture>();
    texture->create(32, 32);

    EntityId entity = registry.createEntity();
    auto& sprite    = registry.emplace<SpriteComponent>(entity);
    sprite.setTexture(texture);
    registry.emplace<TransformComponent>(entity, TransformComponent::create(10.0F, 20.0F, 45.0F));

    renderSystem.update(registry, 0.0F);

    auto raw = sprite.getSprite();
    ASSERT_NE(raw, nullptr);
    EXPECT_FLOAT_EQ(raw->getPosition().x, 10.0F);
    EXPECT_FLOAT_EQ(raw->getPosition().y, 20.0F);
    EXPECT_FLOAT_EQ(raw->getScale().x, 1.0F);
    EXPECT_FLOAT_EQ(raw->getScale().y, 1.0F);
    EXPECT_FLOAT_EQ(raw->getRotation(), 45.0F);
}

TEST(RenderSystem, AppliesScaleAndRotation)
{
    Window window{{64u, 64u}, "Test"};
    RenderSystem renderSystem(window);
    Registry registry;

    auto texture = std::make_shared<SFMLTexture>();
    texture->create(16, 16);

    EntityId entity = registry.createEntity();
    auto& sprite    = registry.emplace<SpriteComponent>(entity);
    sprite.setTexture(texture);
    auto& transform    = registry.emplace<TransformComponent>(entity);
    transform.scaleX   = 2.0F;
    transform.scaleY   = 3.0F;
    transform.rotation = 90.0F;

    renderSystem.update(registry, 0.0F);

    auto raw = sprite.getSprite();
    ASSERT_NE(raw, nullptr);
    EXPECT_FLOAT_EQ(raw->getScale().x, 2.0F);
    EXPECT_FLOAT_EQ(raw->getScale().y, 3.0F);
    EXPECT_FLOAT_EQ(raw->getRotation(), 90.0F);
}

TEST(RenderSystem, RespectsLayerComponentSorting)
{
    Window window{{64u, 64u}, "Test"};
    RenderSystem renderSystem(window);
    Registry registry;

    auto texture = std::make_shared<SFMLTexture>();
    texture->create(16, 16);

    EntityId e1 = registry.createEntity();
    registry.emplace<SpriteComponent>(e1).setTexture(texture);
    registry.emplace<TransformComponent>(e1, TransformComponent::create(1.0F, 1.0F));
    registry.emplace<LayerComponent>(e1, LayerComponent::create(0));

    EntityId e2 = registry.createEntity();
    registry.emplace<SpriteComponent>(e2).setTexture(texture);
    registry.emplace<TransformComponent>(e2, TransformComponent::create(2.0F, 2.0F));
    registry.emplace<LayerComponent>(e2, LayerComponent::create(1));

    auto& s1 = registry.get<SpriteComponent>(e1);
    auto& s2 = registry.get<SpriteComponent>(e2);

    renderSystem.update(registry, 0.0F);

    ASSERT_NE(s1.getSprite(), nullptr);
    ASSERT_NE(s2.getSprite(), nullptr);
    EXPECT_FLOAT_EQ(s1.getSprite()->getPosition().x, 1.0F);
    EXPECT_FLOAT_EQ(s2.getSprite()->getPosition().x, 2.0F);
}

TEST(RenderSystem, UsesDefaultLayerWhenMissing)
{
    Window window{{64u, 64u}, "Test"};
    RenderSystem renderSystem(window);
    Registry registry;

    auto texture = std::make_shared<SFMLTexture>();
    texture->create(16, 16);

    EntityId e = registry.createEntity();
    auto& s    = registry.emplace<SpriteComponent>(e);
    s.setTexture(texture);
    registry.emplace<TransformComponent>(e, TransformComponent::create(3.0F, 4.0F));

    renderSystem.update(registry, 0.0F);

    ASSERT_NE(s.getSprite(), nullptr);
    EXPECT_FLOAT_EQ(s.getSprite()->getPosition().x, 3.0F);
    EXPECT_FLOAT_EQ(s.getSprite()->getPosition().y, 4.0F);
}

TEST(RenderSystem, ThrowsOnMissingTexture)
{
    Window window{{64u, 64u}, "Test"};
    RenderSystem system(window);
    Registry registry;

    auto texture = std::make_shared<SFMLTexture>();
    EntityId e = registry.createEntity();
    
    SpriteComponent s(texture);
    registry.emplace<SpriteComponent>(e, s);
    registry.emplace<TransformComponent>(e, TransformComponent::create(0, 0));

    EXPECT_NO_THROW(system.update(registry, 0.0F));
}

TEST(RenderSystem, UpdatesSpritePosition)
{
    Window window{{64u, 64u}, "Test"};
    RenderSystem system(window);
    Registry registry;
    
    auto texture = std::make_shared<SFMLTexture>();
    texture->create(32, 32);

    EntityId e = registry.createEntity();
    SpriteComponent s(texture);
    registry.emplace<SpriteComponent>(e, s);
    registry.emplace<TransformComponent>(e, TransformComponent::create(3.0F, 4.0F));

    system.update(registry, 0.0F);

    auto& sRef = registry.get<SpriteComponent>(e);
    auto raw = sRef.getSprite();
    ASSERT_NE(raw, nullptr);
    EXPECT_FLOAT_EQ(raw->getPosition().x, 3.0F);
    EXPECT_FLOAT_EQ(raw->getPosition().y, 4.0F);
}

TEST(RenderSystem, IgnoresEntitiesWithoutSpriteInstance)
{
    Window window{{32u, 32u}, "Test"};
    RenderSystem renderSystem(window);
    Registry registry;

    EntityId e = registry.createEntity();
    auto& s    = registry.emplace<SpriteComponent>(e);
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));

    renderSystem.update(registry, 0.0F);
    EXPECT_FALSE(s.hasSprite());
    EXPECT_EQ(s.getSprite(), nullptr);
}

TEST(RenderSystem, SkipsEntitiesWithoutTransformComponent)
{
    Window window{{32u, 32u}, "Test"};
    RenderSystem renderSystem(window);
    Registry registry;

    auto texture = std::make_shared<SFMLTexture>();
    texture->create(8, 8);

    EntityId e = registry.createEntity();
    auto& s    = registry.emplace<SpriteComponent>(e);
    s.setTexture(texture);

    renderSystem.update(registry, 0.0F);
    ASSERT_NE(s.getSprite(), nullptr);
    EXPECT_FLOAT_EQ(s.getSprite()->getPosition().x, 0.0F);
    EXPECT_FLOAT_EQ(s.getSprite()->getPosition().y, 0.0F);
}

TEST(RenderSystem, SkipsDeadEntities)
{
    Window window{{32u, 32u}, "Test"};
    RenderSystem renderSystem(window);
    Registry registry;

    auto texture = std::make_shared<SFMLTexture>();
    texture->create(8, 8);

    EntityId e = registry.createEntity();
    auto& s    = registry.emplace<SpriteComponent>(e);
    s.setTexture(texture);
    registry.emplace<TransformComponent>(e, TransformComponent::create(5.0F, 6.0F));

    registry.destroyEntity(e);

    EXPECT_NO_THROW(renderSystem.update(registry, 0.0F));
}

TEST(RenderSystem, SceneGraphLayeringBackgroundEntitiesHUD)
{
    Window window{{64u, 64u}, "Test"};
    RenderSystem renderSystem(window);
    Registry registry;

    auto texture = std::make_shared<SFMLTexture>();
    texture->create(8, 8);

    EntityId background = registry.createEntity();
    auto& bgSprite      = registry.emplace<SpriteComponent>(background);
    bgSprite.setTexture(texture);
    registry.emplace<TransformComponent>(background);
    registry.emplace<LayerComponent>(background, LayerComponent::create(RenderLayer::Background));

    EntityId player    = registry.createEntity();
    auto& playerSprite = registry.emplace<SpriteComponent>(player);
    playerSprite.setTexture(texture);
    registry.emplace<TransformComponent>(player);
    registry.emplace<LayerComponent>(player, LayerComponent::create(RenderLayer::Entities));

    EntityId hud    = registry.createEntity();
    auto& hudSprite = registry.emplace<SpriteComponent>(hud);
    hudSprite.setTexture(texture);
    registry.emplace<TransformComponent>(hud);
    registry.emplace<LayerComponent>(hud, LayerComponent::create(RenderLayer::HUD));

    EXPECT_NO_THROW(renderSystem.update(registry, 0.0F));

    auto& bgLayer  = registry.get<LayerComponent>(background);
    auto& plLayer  = registry.get<LayerComponent>(player);
    auto& hudLayer = registry.get<LayerComponent>(hud);

    EXPECT_LT(bgLayer.layer, plLayer.layer);
    EXPECT_LT(plLayer.layer, hudLayer.layer);
}
